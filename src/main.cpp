/*
 * main.cpp - main file for QModBus
 *
 * Copyright (c) 2009-2014 Tobias Junghans / Electronic Design Chemnitz
 *
 * This file is part of QModBus - http://qmodbus.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <cstdlib>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTextStream>

#include "climodbusoptions.h"
#include "mainwindow.h"

MainWindow * globalMainWin = NULL;

static const QStringList validDataBits = { QStringLiteral( "7" ), QStringLiteral( "8" ) };
static const QStringList validStopBits = { QStringLiteral( "1" ), QStringLiteral( "1.5" ), QStringLiteral( "2" ) };
static const QStringList validParity   = { QStringLiteral( "none" ), QStringLiteral( "odd" ), QStringLiteral( "even" ) };


static void cliError( const QString & msg )
{
	QTextStream( stderr ) << "Error: " << msg << "\n";
	std::exit( 1 );
}


// Parses the command line into a CliModbusOptions, connecting and switching
// to the requested tab on startup. Prints an error to stderr and exits the
// process on invalid input. Returns options with mode == ModbusCliMode::None
// if no --mode was given, in which case QModBus starts up exactly as before.
static CliModbusOptions parseCliOptions( QCommandLineParser & parser, const QApplication & app )
{
	parser.setApplicationDescription(
		QStringLiteral( "QModBus - Modbus master emulator with GUI" ) );
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption modeOption(
		QStringList() << QStringLiteral( "m" ) << QStringLiteral( "mode" ),
		QStringLiteral( "Connect on startup using Modbus mode <mode>: rtu, tcp or ascii." ),
		QStringLiteral( "mode" ) );
	parser.addOption( modeOption );

	QCommandLineOption serialPortOption(
		QStringLiteral( "serial-port" ),
		QStringLiteral( "Serial device for rtu/ascii mode, e.g. /dev/ttyUSB0 or COM3." ),
		QStringLiteral( "device" ) );
	parser.addOption( serialPortOption );

	QCommandLineOption baudOption(
		QStringLiteral( "baud" ),
		QStringLiteral( "Baud rate for rtu/ascii mode." ),
		QStringLiteral( "rate" ), QStringLiteral( "9600" ) );
	parser.addOption( baudOption );

	QCommandLineOption dataBitsOption(
		QStringLiteral( "data-bits" ),
		QStringLiteral( "Data bits for rtu/ascii mode: 7 or 8." ),
		QStringLiteral( "bits" ), QStringLiteral( "8" ) );
	parser.addOption( dataBitsOption );

	QCommandLineOption stopBitsOption(
		QStringLiteral( "stop-bits" ),
		QStringLiteral( "Stop bits for rtu/ascii mode: 1, 1.5 or 2." ),
		QStringLiteral( "bits" ), QStringLiteral( "1" ) );
	parser.addOption( stopBitsOption );

	QCommandLineOption parityOption(
		QStringLiteral( "parity" ),
		QStringLiteral( "Parity for rtu/ascii mode: none, odd or even." ),
		QStringLiteral( "parity" ), QStringLiteral( "none" ) );
	parser.addOption( parityOption );

	QCommandLineOption hostOption(
		QStringLiteral( "host" ),
		QStringLiteral( "IPv4 address of the Modbus TCP server, e.g. 192.168.1.1." ),
		QStringLiteral( "address" ) );
	parser.addOption( hostOption );

	QCommandLineOption tcpPortOption(
		QStringLiteral( "tcp-port" ),
		QStringLiteral( "TCP port of the Modbus TCP server." ),
		QStringLiteral( "port" ), QStringLiteral( "502" ) );
	parser.addOption( tcpPortOption );

	parser.process( app );

	CliModbusOptions opts;

	const QList<QCommandLineOption> serialOptions = {
		serialPortOption, baudOption, dataBitsOption, stopBitsOption, parityOption
	};
	const QList<QCommandLineOption> tcpOptions = { hostOption, tcpPortOption };

	if( !parser.isSet( modeOption ) )
	{
		for( const QCommandLineOption & opt : serialOptions + tcpOptions )
		{
			if( parser.isSet( opt ) )
				cliError( QStringLiteral( "--%1 requires --mode to be set." ).arg( opt.names().last() ) );
		}
		return opts;
	}

	const QString modeStr = parser.value( modeOption ).toLower();
	if( modeStr == QLatin1String( "rtu" ) )
		opts.mode = ModbusCliMode::Rtu;
	else if( modeStr == QLatin1String( "ascii" ) )
		opts.mode = ModbusCliMode::Ascii;
	else if( modeStr == QLatin1String( "tcp" ) )
		opts.mode = ModbusCliMode::Tcp;
	else
		cliError( QStringLiteral( "invalid --mode '%1', expected rtu, tcp or ascii." ).arg( modeStr ) );

	if( opts.mode == ModbusCliMode::Rtu || opts.mode == ModbusCliMode::Ascii )
	{
		for( const QCommandLineOption & opt : tcpOptions )
		{
			if( parser.isSet( opt ) )
				cliError( QStringLiteral( "--%1 only applies to --mode tcp." ).arg( opt.names().last() ) );
		}

		if( !parser.isSet( serialPortOption ) )
			cliError( QStringLiteral( "--serial-port is required for --mode %1." ).arg( modeStr ) );
		opts.serialPort = parser.value( serialPortOption );

		bool baudOk = false;
		opts.baud = parser.value( baudOption ).toInt( &baudOk );
		if( !baudOk || opts.baud <= 0 )
			cliError( QStringLiteral( "invalid --baud '%1'." ).arg( parser.value( baudOption ) ) );

		if( !validDataBits.contains( parser.value( dataBitsOption ) ) )
			cliError( QStringLiteral( "invalid --data-bits '%1', expected 7 or 8." ).arg( parser.value( dataBitsOption ) ) );
		opts.dataBits = parser.value( dataBitsOption ).toInt();

		if( !validStopBits.contains( parser.value( stopBitsOption ) ) )
			cliError( QStringLiteral( "invalid --stop-bits '%1', expected 1, 1.5 or 2." ).arg( parser.value( stopBitsOption ) ) );
		opts.stopBits = parser.value( stopBitsOption );

		const QString parityStr = parser.value( parityOption ).toLower();
		if( !validParity.contains( parityStr ) )
			cliError( QStringLiteral( "invalid --parity '%1', expected none, odd or even." ).arg( parityStr ) );
		opts.parity = parityStr;
	}
	else // ModbusCliMode::Tcp
	{
		for( const QCommandLineOption & opt : serialOptions )
		{
			if( parser.isSet( opt ) )
				cliError( QStringLiteral( "--%1 only applies to --mode rtu/ascii." ).arg( opt.names().last() ) );
		}

		if( !parser.isSet( hostOption ) )
			cliError( QStringLiteral( "--host is required for --mode tcp." ) );
		opts.host = parser.value( hostOption );

		bool portOk = false;
		opts.tcpPort = parser.value( tcpPortOption ).toInt( &portOk );
		if( !portOk || opts.tcpPort <= 0 || opts.tcpPort > 65535 )
			cliError( QStringLiteral( "invalid --tcp-port '%1'." ).arg( parser.value( tcpPortOption ) ) );
	}

	return opts;
}


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QApplication::setOrganizationName( "EDC Electronic Design Chemnitz GmbH" );
	QApplication::setOrganizationDomain( "ed-chemnitz.de" );
	QApplication::setApplicationName( "QModBus" );
	QApplication::setApplicationVersion( "0.3.0" );

	QCommandLineParser parser;
	const CliModbusOptions cliOptions = parseCliOptions( parser, a );

	MainWindow w( cliOptions );
	w.show();

	globalMainWin = &w;

	return a.exec();
}

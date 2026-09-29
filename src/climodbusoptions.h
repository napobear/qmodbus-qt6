/*
 * climodbusoptions.h - command line configured Modbus connection parameters
 *
 * Copyright (c) 2026 QModBus contributors
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

#ifndef CLIMODBUSOPTIONS_H
#define CLIMODBUSOPTIONS_H

#include <QString>

enum class ModbusCliMode
{
	None,
	Rtu,
	Tcp,
	Ascii
};

// Connection parameters gathered from the command line, applied to the
// GUI once at startup by MainWindow::applyCliOptions().
struct CliModbusOptions
{
	ModbusCliMode mode = ModbusCliMode::None;

	// RTU / ASCII
	QString serialPort;
	int     baud     = 9600;
	int     dataBits = 8;
	QString stopBits = QStringLiteral( "1" );
	QString parity   = QStringLiteral( "none" );

	// TCP
	QString host;
	int     tcpPort  = 502;
};

#endif // CLIMODBUSOPTIONS_H

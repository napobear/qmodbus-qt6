#include <QComboBox>
#include <QSettings>
#include <QSerialPortInfo>
#include <QSignalBlocker>
#include "serialsettingswidget.h"
#include "ui_serialsettingswidget.h"


#ifdef Q_OS_WIN
static inline QString portDisplayName( const QSerialPortInfo & port ) { return port.portName(); }
#else
static inline QString portDisplayName( const QSerialPortInfo & port ) { return port.systemLocation(); }
#endif

SerialSettingsWidget::SerialSettingsWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SerialSettingsWidget),
	m_serialModbus( NULL )
{
	ui->setupUi(this);
	enableGuiItems(false);
}

SerialSettingsWidget::~SerialSettingsWidget()
{
	delete ui;
}

int SerialSettingsWidget::setupModbusPort()
{
	QSettings s;

	int portIndex = 0;
	int i = 0;
    ui->serialPort->disconnect();
    ui->serialPort->clear();
	const auto ports = QSerialPortInfo::availablePorts();
	for( const QSerialPortInfo &port : ports )
	{
		const QString display = portDisplayName( port );
		ui->serialPort->addItem( display );
		if( display == s.value( "serialinterface" ) )
		{
			portIndex = i;
		}
		++i;
	}
	ui->serialPort->setCurrentIndex( portIndex );

	ui->baud->setCurrentIndex( ui->baud->findText( s.value( "serialbaudrate" ).toString() ) );
	ui->parity->setCurrentIndex( ui->parity->findText( s.value( "serialparity" ).toString() ) );
	ui->stopBits->setCurrentIndex( ui->stopBits->findText( s.value( "serialstopbits" ).toString() ) );
	ui->dataBits->setCurrentIndex( ui->dataBits->findText( s.value( "serialdatabits" ).toString() ) );

	connect( ui->serialPort, SIGNAL( currentIndexChanged( int ) ),
			this, SLOT( changeSerialPort( int ) ) );
	connect( ui->baud, SIGNAL( currentIndexChanged( int ) ),
			this, SLOT( changeSerialPort( int ) ) );
	connect( ui->dataBits, SIGNAL( currentIndexChanged( int ) ),
			this, SLOT( changeSerialPort( int ) ) );
	connect( ui->stopBits, SIGNAL( currentIndexChanged( int ) ),
			this, SLOT( changeSerialPort( int ) ) );
	connect( ui->parity, SIGNAL( currentIndexChanged( int ) ),
			this, SLOT( changeSerialPort( int ) ) );

	changeSerialPort( portIndex );
	return portIndex;
}

void SerialSettingsWidget::releaseSerialModbus()
{
	if( m_serialModbus )
	{
		modbus_close( m_serialModbus );
		modbus_free( m_serialModbus );
		m_serialModbus = NULL;
	}
}

static inline QString embracedString( const QString & s )
{
    return s.section( '(', 1 ).section( ')', 0, 0 );
}


void SerialSettingsWidget::changeSerialPort( int )
{
	if( ui->serialPort->count() == 0 )
	{
		emit connectionError( tr( "No serial port found" ) );
		return;
	}

	// Use the combo box's own text rather than re-querying and
	// index-matching QSerialPortInfo::availablePorts(): that list can
	// change between population and selection, and it wouldn't contain
	// a port added explicitly (e.g. via the command line) that isn't
	// currently enumerated by the OS.
	const QString port = ui->serialPort->currentText();

	QSettings settings;
	settings.setValue( "serialinterface", port );
	settings.setValue( "serialbaudrate", ui->baud->currentText() );
	settings.setValue( "serialparity", ui->parity->currentText() );
	settings.setValue( "serialdatabits", ui->dataBits->currentText() );
	settings.setValue( "serialstopbits", ui->stopBits->currentText() );

	QString devicePath = port;
#ifdef Q_OS_WIN
	// is it a serial port in the range COM1 .. COM9?
	if ( devicePath.startsWith( "COM" ) )
	{
		// use windows communication device name "\\.\COMn"
		devicePath = "\\\\.\\" + devicePath;
	}
#endif

	char parity;
	switch( ui->parity->currentIndex() )
	{
		case 1: parity = 'O'; break;
		case 2: parity = 'E'; break;
		default:
		case 0: parity = 'N'; break;
	}

	changeModbusInterface(devicePath, parity);

	emit serialPortActive(true);
}


void SerialSettingsWidget::configureAndActivate( const QString & portName, int baud, int dataBits,
				const QString & stopBits, const QString & parity )
{
	// setupModbusPort() wires each combo box's currentIndexChanged signal
	// to changeSerialPort(), which immediately (re)connects using whatever
	// is currently selected. Block that while we populate every field, so
	// we don't fire off a string of partially-configured connect attempts
	// (e.g. with baud still blank) before all the requested values are in
	// place; the explicit changeSerialPort(0) call below performs the one
	// connection attempt that actually matters.
	const QSignalBlocker blockPort( ui->serialPort );
	const QSignalBlocker blockBaud( ui->baud );
	const QSignalBlocker blockDataBits( ui->dataBits );
	const QSignalBlocker blockStopBits( ui->stopBits );
	const QSignalBlocker blockParity( ui->parity );

	setupModbusPort();

	auto selectOrAdd = []( QComboBox * box, const QString & text )
	{
		int idx = box->findText( text );
		if( idx < 0 )
		{
			box->addItem( text );
			idx = box->count() - 1;
		}
		box->setCurrentIndex( idx );
	};

	if( !portName.isEmpty() )
		selectOrAdd( ui->serialPort, portName );
	if( baud > 0 )
		selectOrAdd( ui->baud, QString::number( baud ) );
	if( dataBits > 0 )
		selectOrAdd( ui->dataBits, QString::number( dataBits ) );
	if( !stopBits.isEmpty() )
		selectOrAdd( ui->stopBits, stopBits );
	if( !parity.isEmpty() )
		selectOrAdd( ui->parity, parity );

	ui->checkBox->setChecked( true );
	enableGuiItems( true );
	changeSerialPort( 0 );
}


void SerialSettingsWidget::enableGuiItems(bool checked)
{
	ui->serialPort->setEnabled(checked);
	ui->baud->setEnabled(checked);
	ui->dataBits->setEnabled(checked);
	ui->stopBits->setEnabled(checked);
	ui->parity->setEnabled(checked);
}

void SerialSettingsWidget::on_checkBox_clicked(bool checked)
{
	if (checked) {
		setupModbusPort();
	}
	else {
		releaseSerialModbus();
	}
	enableGuiItems(checked);
	emit serialPortActive(checked);
}

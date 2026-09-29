#ifndef TCPIPSETTINGSWIDGET_H
#define TCPIPSETTINGSWIDGET_H

#include <QWidget>
#include "imodbus.h"

namespace Ui {
class TcpIpSettingsWidget;
}

class TcpIpSettingsWidget : public QWidget, public IModbus
{
    Q_OBJECT

public:
    TcpIpSettingsWidget(QWidget *parent = 0);
    ~TcpIpSettingsWidget();
    // IModbus interface
    virtual modbus_t *modbus() { return m_tcpModbus; }
    virtual int setupModbusPort();
    void tcpConnect();

    // Fills in address/port (when non-empty/non-zero) and connects, as if
    // the user had entered them and ticked "Active". Used to apply
    // connection parameters given on the command line.
    void configureAndActivate( const QString & address, int port );

protected:
    void changeModbusInterface(const QString& address, int portNbr);
    void releaseTcpModbus();
    void enableGuiItems(bool checked);

private slots:
    void on_cbEnabled_clicked(bool checked);

signals:
    void tcpPortActive(bool val);
    void connectionError(const QString &msg);

private:
    Ui::TcpIpSettingsWidget *ui;
    modbus_t *               m_tcpModbus;
};

#endif // TCPIPSETTINGSWIDGET_H

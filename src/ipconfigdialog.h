#ifndef IPCONFIGDIALOG_H
#define IPCONFIGDIALOG_H

#include <QDialog>
#include <QString>

#include "ipconfig.h"

class QLabel;
class QLineEdit;
class QRadioButton;
class QWidget;

// IPv4 settings for one device — DHCP or a fixed address.
//
// Shared by the WiFi and LAN dialogs: the settings and the NetworkManager
// calls behind them are identical for both link types, so the operator sees
// one form regardless of how the terminal is attached.
class IpConfigDialog : public QDialog {
    Q_OBJECT

public:
    IpConfigDialog(const QString& devicePath, const QString& title,
                   QWidget* parent = nullptr);

private slots:
    void onModeChanged();
    void onSave();

private:
    void loadCurrent();

    QString m_devicePath;

    QRadioButton* m_dhcpRadio;
    QRadioButton* m_staticRadio;
    QWidget* m_staticFields;
    QLineEdit* m_addressEdit;
    QLineEdit* m_netmaskEdit;
    QLineEdit* m_gatewayEdit;
    QLineEdit* m_dnsEdit;
    QLabel* m_errorLabel;
};

#endif

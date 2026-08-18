#ifndef LANDIALOG_H
#define LANDIALOG_H

#include <QDialog>

class NetworkController;
class QLabel;
class QShowEvent;

// Wired network status and settings.
//
// Deliberately thin next to the WiFi dialog: a cable has nothing to choose
// between, so this shows link state and address, and hands off to the shared
// IPv4 settings dialog for DHCP vs fixed address.
class LanDialog : public QDialog {
    Q_OBJECT

public:
    explicit LanDialog(NetworkController* netCtrl, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void updateStatus();

private:
    NetworkController* m_netCtrl;
    QLabel* m_statusLabel;
    QWidget* m_statusBox;
    QLabel* m_detailLabel;
};

#endif

#ifndef WIFIDIALOG_H
#define WIFIDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

class NetworkController;

class WifiDialog : public QDialog {
    Q_OBJECT
public:
    explicit WifiDialog(NetworkController* netCtrl, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void refreshNetworkList();
    void updateStatus();
    void updateScanButton();
    void updateError();
    void onNetworkClicked(QListWidgetItem* item);

private:
    void showPasswordRow(const QString& ssid);
    void showForgetRow(const QString& ssid);
    void hideSubRows();

    NetworkController* m_netCtrl;
    QString m_selectedSsid;

    QLabel* m_statusLabel;
    QWidget* m_statusBox;
    QPushButton* m_scanBtn;
    QListWidget* m_networkList;
    QWidget* m_passwordRow;
    QLineEdit* m_passwordEdit;
    QPushButton* m_connectBtn;
    QWidget* m_forgetRow;
    QLabel* m_forgetLabel;
    QLabel* m_errorLabel;
};

#endif

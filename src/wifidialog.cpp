#include "wifidialog.h"
#include "networkcontroller.h"
#include "dialogstyle.h"
#include <QString>
#include "ipconfigdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVariantMap>

WifiDialog::WifiDialog(NetworkController* netCtrl, QWidget* parent)
    : QDialog(parent)
    , m_netCtrl(netCtrl)
{
    setWindowTitle("WiFi");
    setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());

    auto* layout = new QVBoxLayout(this);

    // --- Header ---
    auto* headerRow = new QHBoxLayout;
    auto* title = new QLabel("WiFi");
    title->setStyleSheet(QString("font-size: %1px; font-weight: bold;")
                             .arg(DialogStyle::px(28)));
    headerRow->addWidget(title);
    headerRow->addStretch();
    // No × in the corner - see landialog.cpp. The Close button in the button
    // row is the way out, and it is the same way out in every dialog.
    layout->addLayout(headerRow);

    // --- Connection status ---
    m_statusBox = new QWidget;
    m_statusBox->setStyleSheet("background: #f2dede; border-radius: 4px; padding: 12px;");
    auto* statusLayout = new QVBoxLayout(m_statusBox);
    statusLayout->setContentsMargins(8, 4, 8, 4);
    m_statusLabel = new QLabel("Not connected");
    m_statusLabel->setStyleSheet("");
    statusLayout->addWidget(m_statusLabel);
    layout->addWidget(m_statusBox);

    // --- Scan row ---
    auto* scanRow = new QHBoxLayout;
    auto* netTitle = new QLabel("Available Networks");
    netTitle->setStyleSheet("font-size: 22px; font-weight: bold;");
    scanRow->addWidget(netTitle);
    scanRow->addStretch();
    m_scanBtn = new QPushButton("Scan");
    m_scanBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(m_scanBtn, &QPushButton::clicked, m_netCtrl, &NetworkController::scan);
    scanRow->addWidget(m_scanBtn);
    layout->addLayout(scanRow);

    // --- Network list ---
    m_networkList = new QListWidget;
    m_networkList->setStyleSheet("background: white;");
    m_networkList->setAlternatingRowColors(true);
    connect(m_networkList, &QListWidget::itemClicked, this, &WifiDialog::onNetworkClicked);
    layout->addWidget(m_networkList, 1);

    // --- Password row (hidden) ---
    m_passwordRow = new QWidget;
    m_passwordRow->setVisible(false);
    auto* pwLayout = new QVBoxLayout(m_passwordRow);
    pwLayout->setContentsMargins(0, 4, 0, 4);
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Password (min 8 chars)");
    m_passwordEdit->setStyleSheet("");

    // Reveal toggle, inside the field at the trailing edge — where every other
    // password field puts it. A WPA key is long, typed on a virtual keyboard,
    // in a packhouse, and a wrong one fails minutes later with "connection
    // failed", which reads as a hardware fault rather than a typo.
    m_revealAction = m_passwordEdit->addAction(QIcon(":/images/eye.svg"),
                                               QLineEdit::TrailingPosition);
    m_revealAction->setCheckable(true);
    m_revealAction->setToolTip("Show password");
    connect(m_revealAction, &QAction::toggled, [this](bool shown) {
        m_passwordEdit->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
        m_revealAction->setIcon(QIcon(shown ? ":/images/eye-off.svg"
                                            : ":/images/eye.svg"));
        m_revealAction->setToolTip(shown ? "Hide password" : "Show password");
    });
    pwLayout->addWidget(m_passwordEdit);

    auto* pwBtnRow = new QHBoxLayout;
    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setEnabled(false);
    m_connectBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(m_connectBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->connectToNetwork(m_selectedSsid, m_passwordEdit->text());
        hideSubRows();
    });
    connect(m_passwordEdit, &QLineEdit::textChanged, [this](const QString& text) {
        m_connectBtn->setEnabled(text.length() >= 8);
    });
    connect(m_passwordEdit, &QLineEdit::returnPressed, [this]() {
        if (m_passwordEdit->text().length() >= 8) {
            m_netCtrl->connectToNetwork(m_selectedSsid, m_passwordEdit->text());
            hideSubRows();
        }
    });
    pwBtnRow->addWidget(m_connectBtn);
    auto* pwCancelBtn = new QPushButton("Cancel");
    pwCancelBtn->setStyleSheet(DialogStyle::Colour::neutral());
    connect(pwCancelBtn, &QPushButton::clicked, [this]() { hideSubRows(); });
    pwBtnRow->addWidget(pwCancelBtn);
    pwBtnRow->addStretch();
    pwLayout->addLayout(pwBtnRow);
    layout->addWidget(m_passwordRow);

    // --- Forget row (hidden) ---
    m_forgetRow = new QWidget;
    m_forgetRow->setVisible(false);
    auto* fgLayout = new QVBoxLayout(m_forgetRow);
    fgLayout->setContentsMargins(0, 4, 0, 4);
    m_forgetLabel = new QLabel;
    m_forgetLabel->setStyleSheet("");
    fgLayout->addWidget(m_forgetLabel);

    auto* fgBtnRow = new QHBoxLayout;
    auto* forgetBtn = new QPushButton("Forget");
    forgetBtn->setStyleSheet(DialogStyle::Colour::danger());
    connect(forgetBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->forgetNetwork(m_selectedSsid);
        hideSubRows();
    });
    fgBtnRow->addWidget(forgetBtn);

    auto* disconnectBtn = new QPushButton("Disconnect");
    disconnectBtn->setStyleSheet(DialogStyle::Colour::warning());
    connect(disconnectBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->disconnectWifi();
        hideSubRows();
    });
    fgBtnRow->addWidget(disconnectBtn);

    auto* fgCancelBtn = new QPushButton("Cancel");
    fgCancelBtn->setStyleSheet(DialogStyle::Colour::neutral());
    connect(fgCancelBtn, &QPushButton::clicked, [this]() { hideSubRows(); });
    fgBtnRow->addWidget(fgCancelBtn);
    fgBtnRow->addStretch();
    fgLayout->addLayout(fgBtnRow);
    layout->addWidget(m_forgetRow);

    // --- Error label ---
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: red;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    layout->addWidget(m_errorLabel);

    // --- Footer ---
    auto* footer = new QHBoxLayout;
    auto* restartBtn = new QPushButton("Restart WiFi");
    restartBtn->setStyleSheet(DialogStyle::Colour::warning());
    connect(restartBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->restartWifi();
        accept();
    });
    footer->addWidget(restartBtn);

    // Same IPv4 form the wired dialog uses: DHCP or a fixed address is a
    // property of the connection, not of how the terminal is attached.
    auto* ipBtn = new QPushButton("IP Settings");
    ipBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(ipBtn, &QPushButton::clicked, [this]() {
        DialogStyle::closeKeyboard();
        IpConfigDialog dlg(m_netCtrl->wifiDevicePath(), "WiFi IP Settings", this);
        dlg.exec();
        updateStatus();
    });
    footer->addWidget(ipBtn);
    footer->addStretch();

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(closeBtn);
    layout->addLayout(footer);

    // --- Signal connections ---
    connect(m_netCtrl, &NetworkController::networksChanged, this, &WifiDialog::refreshNetworkList);
    connect(m_netCtrl, &NetworkController::connectedChanged, this, &WifiDialog::updateStatus);
    connect(m_netCtrl, &NetworkController::ssidChanged, this, &WifiDialog::updateStatus);
    connect(m_netCtrl, &NetworkController::ipAddressChanged, this, &WifiDialog::updateStatus);
    connect(m_netCtrl, &NetworkController::scanningChanged, this, &WifiDialog::updateScanButton);
    connect(m_netCtrl, &NetworkController::errorChanged, this, &WifiDialog::updateError);

    DialogStyle::takeNoFocusExceptFields(this);
}

void WifiDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    DialogStyle::sizeToScreen(this, 0.92, 0.72);
    resize(minimumSize());
    DialogStyle::centerOnScreen(this);
    // Reset state
    m_selectedSsid.clear();
    hideSubRows();
    updateStatus();
    updateError();
    m_netCtrl->scan();
}

void WifiDialog::refreshNetworkList()
{
    m_networkList->clear();
    QVariantList nets = m_netCtrl->networks();
    for (const QVariant& v : nets) {
        QVariantMap net = v.toMap();
        QString ssid = net["ssid"].toString();
        int signal = net["signal"].toInt();
        QString security = net["security"].toString();
        bool isConnected = net["connected"].toBool();

        // Build signal bars: 4 bars, filled based on signal/25
        int bars = qMin(signal / 25, 4);
        QString barStr;
        for (int i = 0; i < 4; i++)
            barStr += (i < bars) ? QString::fromUtf8("\xe2\x96\x88") : QString::fromUtf8("\xe2\x96\x91"); // █ vs ░

        QString text = QString("%1%2  %3  %4")
            .arg(isConnected ? QString::fromUtf8("\xe2\x9c\x93 ") : "  ")  // ✓
            .arg(ssid)
            .arg(barStr)
            .arg(security);

        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, ssid);
        item->setData(Qt::UserRole + 1, isConnected);
        item->setData(Qt::UserRole + 2, net["saved"].toBool());
        item->setData(Qt::UserRole + 3, security);

        if (isConnected)
            item->setBackground(QColor("#e8f5e9"));

        m_networkList->addItem(item);
    }
}

void WifiDialog::updateStatus()
{
    // The provisioning hotspot reads as "connected" to NetworkManager, but the
    // terminal has no uplink at all while it is up — the radio cannot run AP
    // and station mode at once. Say so, rather than showing the setup SSID as
    // though the operator were on the network.
    if (m_netCtrl->hotspotActive()) {
        m_statusBox->setStyleSheet("background: #fcf8e3; border-radius: 4px; padding: 12px;");
        QString text = QString("Setup mode: %1").arg(m_netCtrl->ssid());
        text += "\nNot connected to a network. Choose one below to go online.";
        m_statusLabel->setText(text);
    } else if (m_netCtrl->connected()) {
        m_statusBox->setStyleSheet("background: #dff0d8; border-radius: 4px; padding: 12px;");
        QString text = QString("Connected: %1").arg(m_netCtrl->ssid());
        if (!m_netCtrl->ipAddress().isEmpty())
            text += QString("\nIP: %1").arg(m_netCtrl->ipAddress());
        m_statusLabel->setText(text);
    } else {
        m_statusBox->setStyleSheet("background: #f2dede; border-radius: 4px; padding: 12px;");
        m_statusLabel->setText("Not connected");
    }
}

void WifiDialog::updateScanButton()
{
    if (m_netCtrl->scanning()) {
        m_scanBtn->setText("Scanning...");
        m_scanBtn->setEnabled(false);
    } else {
        m_scanBtn->setText("Scan");
        m_scanBtn->setEnabled(true);
    }
}

void WifiDialog::updateError()
{
    QString err = m_netCtrl->error();
    m_errorLabel->setVisible(!err.isEmpty());
    m_errorLabel->setText(err);
}

void WifiDialog::onNetworkClicked(QListWidgetItem* item)
{
    QString ssid = item->data(Qt::UserRole).toString();
    bool isConnected = item->data(Qt::UserRole + 1).toBool();
    bool isSaved = item->data(Qt::UserRole + 2).toBool();
    QString security = item->data(Qt::UserRole + 3).toString();

    m_selectedSsid = ssid;

    if (isConnected) {
        showForgetRow(ssid);
    } else if (security == "Open" || isSaved) {
        m_netCtrl->connectToNetwork(ssid, "");
        hideSubRows();
    } else {
        showPasswordRow(ssid);
    }
}

void WifiDialog::showPasswordRow(const QString& /* ssid */)
{
    m_forgetRow->setVisible(false);
    m_passwordEdit->clear();
    // Start hidden every time, so a key left revealed does not stay on screen
    // for the next network.
    m_revealAction->setChecked(false);
    m_connectBtn->setEnabled(false);
    m_passwordRow->setVisible(true);
    m_passwordEdit->setFocus();
}

void WifiDialog::showForgetRow(const QString& ssid)
{
    m_passwordRow->setVisible(false);
    m_forgetLabel->setText(QString("Forget \"%1\"?").arg(ssid));
    m_forgetRow->setVisible(true);
}

void WifiDialog::hideSubRows()
{
    m_passwordRow->setVisible(false);
    m_forgetRow->setVisible(false);
    m_passwordEdit->clear();
}

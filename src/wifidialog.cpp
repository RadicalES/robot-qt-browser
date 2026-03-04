#include "wifidialog.h"
#include "networkcontroller.h"

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
    setStyleSheet("QDialog { background-color: #f0f0f0; }");

    auto* layout = new QVBoxLayout(this);

    // --- Header ---
    auto* headerRow = new QHBoxLayout;
    auto* title = new QLabel("WiFi");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    headerRow->addWidget(title);
    headerRow->addStretch();
    auto* closeHeaderBtn = new QPushButton(QString::fromUtf8("\xc3\x97")); // ×
    closeHeaderBtn->setFixedSize(28, 28);
    closeHeaderBtn->setStyleSheet("font-size: 18px; border: none; background: transparent;");
    connect(closeHeaderBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerRow->addWidget(closeHeaderBtn);
    layout->addLayout(headerRow);

    // --- Connection status ---
    m_statusBox = new QWidget;
    m_statusBox->setStyleSheet("background: #f2dede; border-radius: 4px; padding: 6px;");
    auto* statusLayout = new QVBoxLayout(m_statusBox);
    statusLayout->setContentsMargins(8, 4, 8, 4);
    m_statusLabel = new QLabel("Not connected");
    m_statusLabel->setStyleSheet("font-size: 13px;");
    statusLayout->addWidget(m_statusLabel);
    layout->addWidget(m_statusBox);

    // --- Scan row ---
    auto* scanRow = new QHBoxLayout;
    auto* netTitle = new QLabel("Available Networks");
    netTitle->setStyleSheet("font-size: 14px; font-weight: bold;");
    scanRow->addWidget(netTitle);
    scanRow->addStretch();
    m_scanBtn = new QPushButton("Scan");
    m_scanBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
    connect(m_scanBtn, &QPushButton::clicked, m_netCtrl, &NetworkController::scan);
    scanRow->addWidget(m_scanBtn);
    layout->addLayout(scanRow);

    // --- Network list ---
    m_networkList = new QListWidget;
    m_networkList->setStyleSheet("font-size: 13px; background: white;");
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
    m_passwordEdit->setStyleSheet("font-size: 13px; padding: 4px;");
    pwLayout->addWidget(m_passwordEdit);

    auto* pwBtnRow = new QHBoxLayout;
    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setEnabled(false);
    m_connectBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
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
    pwCancelBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
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
    m_forgetLabel->setStyleSheet("font-size: 13px;");
    fgLayout->addWidget(m_forgetLabel);

    auto* fgBtnRow = new QHBoxLayout;
    auto* forgetBtn = new QPushButton("Forget");
    forgetBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
    connect(forgetBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->forgetNetwork(m_selectedSsid);
        hideSubRows();
    });
    fgBtnRow->addWidget(forgetBtn);

    auto* disconnectBtn = new QPushButton("Disconnect");
    disconnectBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
    connect(disconnectBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->disconnectWifi();
        hideSubRows();
    });
    fgBtnRow->addWidget(disconnectBtn);

    auto* fgCancelBtn = new QPushButton("Cancel");
    fgCancelBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
    connect(fgCancelBtn, &QPushButton::clicked, [this]() { hideSubRows(); });
    fgBtnRow->addWidget(fgCancelBtn);
    fgBtnRow->addStretch();
    fgLayout->addLayout(fgBtnRow);
    layout->addWidget(m_forgetRow);

    // --- Error label ---
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: red; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    layout->addWidget(m_errorLabel);

    // --- Footer ---
    auto* footer = new QHBoxLayout;
    auto* restartBtn = new QPushButton("Restart WiFi");
    restartBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
    connect(restartBtn, &QPushButton::clicked, [this]() {
        m_netCtrl->restartWifi();
        accept();
    });
    footer->addWidget(restartBtn);
    footer->addStretch();
    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet("font-size: 12px; padding: 4px 12px;");
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
}

void WifiDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // Size to parent
    if (parentWidget()) {
        int w = qMin(static_cast<int>(parentWidget()->width() * 0.85), 420);
        int h = qMin(static_cast<int>(parentWidget()->height() * 0.8), 520);
        setFixedSize(w, h);
    }
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
    if (m_netCtrl->connected()) {
        m_statusBox->setStyleSheet("background: #dff0d8; border-radius: 4px; padding: 6px;");
        QString text = QString("Connected: %1").arg(m_netCtrl->ssid());
        if (!m_netCtrl->ipAddress().isEmpty())
            text += QString("\nIP: %1").arg(m_netCtrl->ipAddress());
        m_statusLabel->setText(text);
    } else {
        m_statusBox->setStyleSheet("background: #f2dede; border-radius: 4px; padding: 6px;");
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

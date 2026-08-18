#include "landialog.h"
#include "networkcontroller.h"
#include "ipconfigdialog.h"
#include "dialogstyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>

LanDialog::LanDialog(NetworkController* netCtrl, QWidget* parent)
    : QDialog(parent)
    , m_netCtrl(netCtrl)
{
    setWindowTitle("Wired Network");
    setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());
    DialogStyle::widthToScreen(this, 0.94);

    auto* layout = new QVBoxLayout(this);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel("Wired Network");
    title->setStyleSheet("font-size: 28px; font-weight: bold;");
    header->addWidget(title);
    header->addStretch();
    auto* closeHeaderBtn = new QPushButton(QString::fromUtf8("\xc3\x97"));
    closeHeaderBtn->setFixedSize(56, 56);
    closeHeaderBtn->setStyleSheet("font-size: 30px; border: none; background: transparent;"
                                 "min-width: 0px; padding: 0px;");
    connect(closeHeaderBtn, &QPushButton::clicked, this, &QDialog::accept);
    header->addWidget(closeHeaderBtn);
    layout->addLayout(header);

    m_statusBox = new QWidget;
    auto* statusLayout = new QVBoxLayout(m_statusBox);
    m_statusLabel = new QLabel;
    statusLayout->addWidget(m_statusLabel);
    m_detailLabel = new QLabel;
    statusLayout->addWidget(m_detailLabel);
    layout->addWidget(m_statusBox);

    auto* buttons = new QHBoxLayout;
    auto* ipBtn = new QPushButton("IP Settings");
    ipBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(ipBtn, &QPushButton::clicked, [this]() {
        DialogStyle::closeKeyboard();
        IpConfigDialog dlg(m_netCtrl->lanDevicePath(), "Wired IP Settings", this);
        dlg.exec();
        updateStatus();
    });
    buttons->addWidget(ipBtn);
    buttons->addStretch();
    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(closeBtn);
    layout->addLayout(buttons);

    connect(m_netCtrl, &NetworkController::lanChanged, this, &LanDialog::updateStatus);

    DialogStyle::takeNoFocusExceptFields(this);
}

void LanDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    updateStatus();
}

void LanDialog::updateStatus()
{
    if (!m_netCtrl->lanAvailable()) {
        m_statusLabel->setText("No wired interface");
        m_detailLabel->setText("This terminal has no ethernet port, or the "
                               "driver did not load.");
        m_statusBox->setStyleSheet("background: #f2dede; border-radius: 4px; padding: 12px;");
        return;
    }

    if (m_netCtrl->lanConnected()) {
        m_statusLabel->setText("Connected");
        m_detailLabel->setText("IP: " + m_netCtrl->lanIpAddress());
        m_statusBox->setStyleSheet("background: #dff0d8; border-radius: 4px; padding: 12px;");
    } else if (m_netCtrl->lanCarrier()) {
        // Cable in, no address: almost always DHCP with nothing answering, or
        // a fixed address that does not suit the network it is plugged into.
        m_statusLabel->setText("Cable connected, no address");
        m_detailLabel->setText("Check IP Settings, or the network's DHCP server.");
        m_statusBox->setStyleSheet("background: #fcf8e3; border-radius: 4px; padding: 12px;");
    } else {
        m_statusLabel->setText("No cable");
        m_detailLabel->setText("Plug in a network cable.");
        m_statusBox->setStyleSheet("background: #f2dede; border-radius: 4px; padding: 12px;");
    }
}

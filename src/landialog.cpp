#include "landialog.h"
#include "networkcontroller.h"
#include "ipconfigdialog.h"
#include "dialogstyle.h"
#include <QString>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QShowEvent>

namespace {

// The wired interface's MAC, straight from sysfs.
//
// eth*/en* in name order, which is the wired port on these terminals — the
// same rule the SCADA indicator uses to report the MAC a server knows this
// terminal by, so the two agree.
QString wiredMacAddress()
{
    const QStringList ifaces = QDir("/sys/class/net").entryList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System, QDir::Name);

    for (const QString& iface : ifaces) {
        if (!iface.startsWith("eth") && !iface.startsWith("en"))
            continue;
        QFile file("/sys/class/net/" + iface + "/address");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QString mac = QString::fromLatin1(file.readAll()).trimmed();
        if (!mac.isEmpty())
            return mac.toUpper();
    }
    return QStringLiteral("unavailable");
}

} // namespace

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
    title->setStyleSheet(QString("font-size: %1px; font-weight: bold;")
                             .arg(DialogStyle::px(28)));
    header->addWidget(title);
    header->addStretch();
    auto* closeHeaderBtn = new QPushButton(QString::fromUtf8("\xc3\x97"));
    closeHeaderBtn->setFixedSize(DialogStyle::px(56), DialogStyle::px(56));
    closeHeaderBtn->setStyleSheet(QString("font-size: %1px; border: none;"
                                          "background: transparent;"
                                          "min-width: 0px; padding: 0px;")
                                      .arg(DialogStyle::px(30)));
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

    // The MAC, always shown, connected or not.
    //
    // It is what a site needs to give this terminal a DHCP reservation, and
    // that is exactly the job someone is doing when the cable is in and the
    // address is wrong — or not in yet at all. Reading it from sysfs rather
    // than NetworkManager means it is there even when the link is down.
    auto* macLabel = new QLabel("MAC: " + wiredMacAddress());
    macLabel->setStyleSheet(QString("color: #555; font-size: %1px;")
                                .arg(DialogStyle::px(18)));
    macLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(macLabel);

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

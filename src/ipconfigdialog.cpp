#include "ipconfigdialog.h"
#include "dialogstyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>

IpConfigDialog::IpConfigDialog(const QString& devicePath, const QString& title,
                               QWidget* parent)
    : QDialog(parent)
    , m_devicePath(devicePath)
{
    setWindowTitle(title);
    setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());
    DialogStyle::widthToScreen(this, 0.94);

    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel(title);
    header->setStyleSheet("font-size: 28px; font-weight: bold;");
    layout->addWidget(header);

    m_dhcpRadio = new QRadioButton("Automatic (DHCP)");
    m_staticRadio = new QRadioButton("Fixed address");
    auto* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_dhcpRadio);
    modeGroup->addButton(m_staticRadio);
    layout->addWidget(m_dhcpRadio);
    layout->addWidget(m_staticRadio);

    m_staticFields = new QWidget;
    auto* form = new QFormLayout(m_staticFields);
    form->setContentsMargins(0, 0, 0, 0);
    m_addressEdit = new QLineEdit;
    m_netmaskEdit = new QLineEdit;
    m_gatewayEdit = new QLineEdit;
    m_dnsEdit = new QLineEdit;
    m_addressEdit->setPlaceholderText("192.168.1.50");
    m_netmaskEdit->setPlaceholderText("255.255.255.0");
    m_gatewayEdit->setPlaceholderText("192.168.1.1");
    m_dnsEdit->setPlaceholderText("192.168.1.1, 8.8.8.8");
    form->addRow("IP address", m_addressEdit);
    form->addRow("Netmask", m_netmaskEdit);
    form->addRow("Gateway", m_gatewayEdit);
    form->addRow("DNS", m_dnsEdit);
    layout->addWidget(m_staticFields);

    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet("color: red;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    auto* buttons = new QHBoxLayout;
    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet(DialogStyle::Colour::neutral());
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(cancelBtn);
    buttons->addStretch();
    auto* saveBtn = new QPushButton("Save");
    saveBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(saveBtn, &QPushButton::clicked, this, &IpConfigDialog::onSave);
    buttons->addWidget(saveBtn);
    layout->addLayout(buttons);

    connect(m_dhcpRadio, &QRadioButton::toggled, this, &IpConfigDialog::onModeChanged);

    loadCurrent();
    DialogStyle::takeNoFocusExceptFields(this);
}

void IpConfigDialog::loadCurrent()
{
    const Ipv4Config config = NetworkIp::readActive(m_devicePath);

    m_dhcpRadio->setChecked(config.automatic);
    m_staticRadio->setChecked(!config.automatic);

    // Pre-fill from what the device is running with, even under DHCP: an
    // operator switching to a fixed address usually wants the current one as a
    // starting point rather than a blank form.
    m_addressEdit->setText(config.address);
    m_netmaskEdit->setText(NetworkIp::prefixToNetmask(config.prefix));
    m_gatewayEdit->setText(config.gateway);
    m_dnsEdit->setText(config.dns);

    onModeChanged();
}

void IpConfigDialog::onModeChanged()
{
    m_staticFields->setEnabled(m_staticRadio->isChecked());
}

void IpConfigDialog::onSave()
{
    Ipv4Config config;
    config.automatic = m_dhcpRadio->isChecked();

    if (!config.automatic) {
        config.address = m_addressEdit->text().trimmed();
        config.gateway = m_gatewayEdit->text().trimmed();
        config.dns = m_dnsEdit->text().trimmed();

        const int prefix = NetworkIp::netmaskToPrefix(m_netmaskEdit->text().trimmed());
        if (prefix < 0) {
            m_errorLabel->setText("Netmask is not valid — for example 255.255.255.0");
            m_errorLabel->show();
            return;
        }
        config.prefix = prefix;

        if (config.address.isEmpty()) {
            m_errorLabel->setText("Enter an IP address");
            m_errorLabel->show();
            return;
        }
    }

    QString error;
    if (!NetworkIp::apply(m_devicePath, config, &error)) {
        m_errorLabel->setText(error);
        m_errorLabel->show();
        return;
    }
    accept();
}

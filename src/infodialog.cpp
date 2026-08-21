#include "infodialog.h"
#include "rebootdialog.h"
#include "systemcontroller.h"
#include "dialogstyle.h"
#include "robothead.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

InfoDialog::InfoDialog(SystemController* sysCtrl, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("System Info");
    setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());
    DialogStyle::widthToScreen(this, 0.94);

    auto* layout = new QVBoxLayout(this);

    // Header: mascot plus title. The head carries the SCADA state, so the
    // dialog says at a glance what the toolbar indicator says.
    auto* headerRow = new QHBoxLayout;

    m_head = new QLabel;
    m_head->setFixedSize(56, 56);
    setStatus(RobotHead::Off);
    headerRow->addWidget(m_head);

    auto* header = new QLabel("System Info");
    header->setStyleSheet("font-size: 30px; font-weight: bold;");
    headerRow->addWidget(header);
    headerRow->addStretch();

    layout->addLayout(headerRow);

    // System info text
    auto* infoText = new QLabel(sysCtrl->systemInfo());
    infoText->setStyleSheet(
        "font-family: monospace; font-size: 19px; background: white; "
        "border: 1px solid #ccc; border-radius: 6px; padding: 14px;");
    infoText->setWordWrap(true);
    infoText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(infoText);

    // Action buttons
    auto* buttonRow = new QHBoxLayout;

    auto* resetBtn = new QPushButton("Reset Defaults");
    m_resetBtn = resetBtn;
    resetBtn->setStyleSheet(
        DialogStyle::Colour::danger());
    connect(resetBtn, &QPushButton::clicked, [sysCtrl]() {
        sysCtrl->resetDefaults();
    });
    buttonRow->addWidget(resetBtn);

    buttonRow->addStretch();

    auto* rebootBtn = new QPushButton("Reboot");
    m_rebootBtn = rebootBtn;
    rebootBtn->setStyleSheet(
        DialogStyle::Colour::warning());
    connect(rebootBtn, &QPushButton::clicked, [this, sysCtrl]() {
        DialogStyle::closeKeyboard();
        RebootDialog dlg(sysCtrl, this);
        dlg.exec();
    });
    buttonRow->addWidget(rebootBtn);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        DialogStyle::Colour::primary());
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonRow->addWidget(closeBtn);

    layout->addLayout(buttonRow);

    DialogStyle::takeNoFocusExceptFields(this);
}

void InfoDialog::setStatus(RobotHead::Variant variant)
{
    m_head->setPixmap(RobotHead::pixmap(56, variant));
}

void InfoDialog::setSystemActionsVisible(bool visible)
{
    m_resetBtn->setVisible(visible);
    m_rebootBtn->setVisible(visible);
}

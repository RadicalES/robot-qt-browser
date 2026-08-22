#include "infodialog.h"
#include "rebootdialog.h"
#include "systemcontroller.h"
#include "dialogstyle.h"
#include <QString>
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
    m_head->setFixedSize(DialogStyle::px(56), DialogStyle::px(56));
    // The product mark, not the SCADA state. This used to be recoloured from
    // the connection, so opening System Info on an unprovisioned terminal
    // showed an orange-then-grey face that looked like a warning about
    // whatever the operator had just clicked.
    // The pixmap has to scale with the label, not just the label: rendering
    // at 56 into a 44px label is how the head came out clipped.
    m_head->setPixmap(RobotHead::pixmap(DialogStyle::px(56), RobotHead::Standard));
    headerRow->addWidget(m_head);

    auto* header = new QLabel("System Info");
    header->setStyleSheet(QString("font-size: %1px; font-weight: bold;")
                              .arg(DialogStyle::px(30)));
    headerRow->addWidget(header);
    headerRow->addStretch();

    layout->addLayout(headerRow);

    // System info text
    auto* infoText = new QLabel(sysCtrl->systemInfo());
    infoText->setStyleSheet(
        QString("font-family: monospace; font-size: %1px; background: white; ")
            .arg(DialogStyle::px(19)) +
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



void InfoDialog::setSystemActionsVisible(bool visible)
{
    m_resetBtn->setVisible(visible);
    m_rebootBtn->setVisible(visible);
}

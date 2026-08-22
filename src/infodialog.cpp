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

    // Who to call. A terminal in a packhouse is a long way from whoever
    // supports it, and this dialog is the one place an operator already goes
    // when something is wrong — so the number belongs here, not in a manual
    // nobody has to hand.
    auto* contact = new QLabel(
        "Radical Electronic Systems\n"
        "http://www.radicalsystems.co.za\n"
        "Tel: +27 76 224 2224");
    contact->setStyleSheet(QString("color: #555; font-size: %1px;")
                               .arg(DialogStyle::px(18)));
    contact->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(contact);

    // Action buttons
    auto* buttonRow = new QHBoxLayout;
    m_buttonRow = buttonRow;

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



void InfoDialog::addSettingsButton(const std::function<void()>& open)
{
    auto* button = new QPushButton("Settings…");
    button->setStyleSheet(DialogStyle::Colour::neutral());
    connect(button, &QPushButton::clicked, this, [this, open]() {
        // Close first: the settings dialog is modal on this one, and two stacked
        // modal dialogs is how the input method ends up pointed at a window
        // nobody can see.
        accept();
        open();
    });
    m_buttonRow->insertWidget(0, button);
}

void InfoDialog::setSystemActionsVisible(bool visible)
{
    m_resetBtn->setVisible(visible);
    m_rebootBtn->setVisible(visible);
}

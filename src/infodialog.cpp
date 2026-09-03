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
#include <QVBoxLayout>

#include "pagezoom.h"

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
    m_layout = layout;

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



void InfoDialog::addZoomRow(qreal current, const std::function<void(qreal)>& apply)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(DialogStyle::px(10));

    auto* caption = new QLabel("Text size");
    caption->setStyleSheet(QString("font-size: %1px;").arg(DialogStyle::px(19)));
    row->addWidget(caption);
    row->addStretch();

    // The size as a percentage, because that is how everyone reads a zoom, and
    // wide enough that the number changing does not move the buttons under the
    // finger pressing them.
    m_zoomLabel = new QLabel;
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(DialogStyle::px(90));
    m_zoomLabel->setStyleSheet(
        QString("font-size: %1px; font-weight: 600;").arg(DialogStyle::px(22)));

    // Big enough to hit with a thumb on a panel, which the ordinary dialog
    // buttons are not - they are read and pressed once, these are pressed
    // repeatedly while looking past the dialog at the page behind it.
    const int side = DialogStyle::px(56);
    auto* smaller = new QPushButton("A-");
    auto* larger = new QPushButton("A+");
    for (QPushButton* b : { smaller, larger }) {
        b->setFixedSize(side, side);
        b->setStyleSheet(DialogStyle::Colour::neutral() +
                         QString("font-size: %1px; font-weight: 600;")
                             .arg(DialogStyle::px(22)));
    }

    auto show = [this](qreal zoom) {
        m_zoomLabel->setText(QString::number(int(zoom * 100 + 0.5)) + "%");
    };
    show(current);

    // The current size lives in the lambdas rather than in a member: this row
    // is the only thing that changes it, and the page is the only thing that
    // has to agree.
    auto* held = new qreal(current);
    connect(smaller, &QPushButton::clicked, this, [held, show, apply]() {
        *held = PageZoom::stepDown(*held);
        show(*held);
        apply(*held);
    });
    connect(larger, &QPushButton::clicked, this, [held, show, apply]() {
        *held = PageZoom::stepUp(*held);
        show(*held);
        apply(*held);
    });
    connect(this, &QObject::destroyed, [held]() { delete held; });

    row->addWidget(smaller);
    row->addWidget(m_zoomLabel);
    row->addWidget(larger);

    // Above the buttons that close, reboot and reset: it is a setting, not an
    // action, and it should not sit among things that end the conversation.
    m_layout->insertLayout(m_layout->indexOf(m_buttonRow), row);
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

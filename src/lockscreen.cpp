#include "lockscreen.h"

#include "dialogstyle.h"
#include "robothead.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

LockScreen::LockScreen(QWidget* parent)
    : QWidget(parent)
{
    const int pad = DialogStyle::px(24);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(pad, pad, pad, pad);
    layout->setSpacing(DialogStyle::px(14));
    layout->addStretch(1);

    // The product mark, at the size the Info dialog uses. A locked terminal is
    // the screen a site sees most of, so it should look like the product
    // rather than like an error.
    QLabel* mark = new QLabel(this);
    mark->setPixmap(RobotHead::pixmap(DialogStyle::px(72), RobotHead::Standard));
    mark->setAlignment(Qt::AlignCenter);
    layout->addWidget(mark);

    m_station = new QLabel(this);
    m_station->setAlignment(Qt::AlignCenter);
    m_station->setObjectName("station");
    layout->addWidget(m_station);

    m_prompt = new QLabel(this);
    m_prompt->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_prompt);

    // How the key arrives. Only shown when there is a choice to make: a
    // terminal with no reader has one way in, and a row of one button is
    // furniture.
    m_entryRow = new QWidget(this);
    QHBoxLayout* methods = new QHBoxLayout(m_entryRow);
    methods->setContentsMargins(0, 0, 0, 0);
    methods->setSpacing(DialogStyle::px(8));
    methods->addStretch(1);

    m_keypadButton = new QPushButton(tr("Enter code"), m_entryRow);
    m_cardButton = new QPushButton(tr("Present card"), m_entryRow);
    for (QPushButton* button : {m_keypadButton, m_cardButton}) {
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QPushButton::clicked, this, &LockScreen::onMethodChanged);
        methods->addWidget(button);
    }
    methods->addStretch(1);
    layout->addWidget(m_entryRow);

    m_key = new QLineEdit(this);
    m_key->setAlignment(Qt::AlignCenter);
    m_key->setEchoMode(QLineEdit::Password);
    m_key->setMaxLength(64);
    connect(m_key, &QLineEdit::returnPressed, this, &LockScreen::onSubmit);
    layout->addWidget(m_key, 0, Qt::AlignCenter);

    m_signOn = new QPushButton(tr("Sign on"), this);
    connect(m_signOn, &QPushButton::clicked, this, &LockScreen::onSubmit);
    layout->addWidget(m_signOn, 0, Qt::AlignCenter);

    m_message = new QLabel(this);
    m_message->setAlignment(Qt::AlignCenter);
    m_message->setWordWrap(true);
    m_message->setObjectName("message");
    layout->addWidget(m_message);

    layout->addStretch(2);

    applyStyle();
    setMethod(Keypad);
}

void LockScreen::applyStyle()
{
    // Black, like the view behind it, so locking does not flash a different
    // colour at somebody walking past.
    setStyleSheet(QString(
        "LockScreen { background: black; }"
        "QLabel { color: #d9d9d9; font-size: %1px; }"
        "QLabel#station { color: #ffffff; font-size: %2px; font-weight: bold; }"
        "QLabel#message { color: #ff9800; font-size: %3px; }"
        "QLineEdit { background: #1a1a1a; color: #ffffff; border: %4px solid #4d4d4d;"
        "  border-radius: %5px; padding: %6px; font-size: %7px; min-width: %8px; }"
        "QLineEdit:focus { border-color: #ff9800; }"
        "QPushButton { background: #4d4d4d; color: #ffffff; border: none;"
        "  border-radius: %5px; padding: %6px %9px; font-size: %3px; }"
        "QPushButton:checked { background: #ff9800; color: #1a1a1a; }"
        "QPushButton:pressed { background: #ff9800; color: #1a1a1a; }")
        .arg(DialogStyle::px(20))
        .arg(DialogStyle::px(26))
        .arg(DialogStyle::px(18))
        .arg(DialogStyle::px(2))
        .arg(DialogStyle::px(6))
        .arg(DialogStyle::px(10))
        .arg(DialogStyle::px(28))
        .arg(DialogStyle::px(260))
        .arg(DialogStyle::px(24)));
}

void LockScreen::setStation(const QString& station)
{
    m_station->setText(station.isEmpty() ? tr("Terminal locked") : station);
}

void LockScreen::setCardAvailable(bool available)
{
    if (m_cardAvailable == available)
        return;

    m_cardAvailable = available;
    m_cardButton->setVisible(available);
    m_entryRow->setVisible(available);

    // A terminal whose reader has just come back should offer it, but not by
    // yanking the operator out of a code they are half-way through typing.
    if (!available && m_method == Card)
        setMethod(Keypad);
    else if (available && m_key->text().isEmpty())
        setMethod(Card);
}

void LockScreen::setMethod(Method method)
{
    m_method = method;
    m_keypadButton->setChecked(method == Keypad);
    m_cardButton->setChecked(method == Card);

    // The key is hidden for a card, because there is nothing to type and a
    // field the operator cannot fill invites them to try.
    const bool typing = (method == Keypad);
    m_key->setVisible(typing);
    m_signOn->setVisible(typing);
    m_prompt->setText(typing ? tr("Enter your worker code")
                             : tr("Present your card to the reader"));

    if (typing) {
        m_key->clear();
        m_key->setFocus();
    }
}

void LockScreen::onMethodChanged()
{
    clearMessage();
    setMethod(m_cardButton->isChecked() ? Card : Keypad);
}

void LockScreen::onSubmit()
{
    const QString key = m_key->text().trimmed();
    if (key.isEmpty()) {
        showError(tr("Enter your worker code"));
        return;
    }

    submitKey(key);
}

void LockScreen::submitKey(const QString& key)
{
    if (key.isEmpty())
        return;

    m_key->clear();
    showBusy(tr("Signing on…"));
    emit keyEntered(key);
}

void LockScreen::showError(const QString& message)
{
    m_message->setStyleSheet(QString("color: #ff5252; font-size: %1px;")
                                     .arg(DialogStyle::px(18)));
    m_message->setText(message);

    // Whatever went wrong, the next worker starts from a clean field.
    if (m_method == Keypad)
        m_key->setFocus();
}

void LockScreen::showBusy(const QString& message)
{
    m_message->setStyleSheet(QString("color: #d9d9d9; font-size: %1px;")
                                     .arg(DialogStyle::px(18)));
    m_message->setText(message);
}

void LockScreen::clearMessage()
{
    m_message->clear();
}

void LockScreen::reset()
{
    clearMessage();
    m_key->clear();
    if (m_method == Keypad)
        m_key->setFocus();
}

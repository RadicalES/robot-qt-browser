#include "lockscreen.h"

#include <QPainter>
#include <QResizeEvent>
#include <QPixmap>
#include <QPushButton>

#include "dialogstyle.h"
#include "robothead.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {

// The shared icons are stroked in #4d4d4d, drawn for the light dialogs. On the
// lock screen's black they are all but invisible, so they are recoloured
// rather than duplicated: one eye in the resources, tinted for wherever it is
// used.
QIcon tinted(const QString& path, const QColor& colour, int size)
{
    QPixmap source = QIcon(path).pixmap(size, size);
    if (source.isNull())
        return QIcon(path);

    QPixmap result(source.size());
    result.setDevicePixelRatio(source.devicePixelRatio());
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.drawPixmap(0, 0, source);
    // Keeps the icon's shape and replaces its colour.
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), colour);
    painter.end();

    return QIcon(result);
}

}  // namespace

LockScreen::LockScreen(QWidget* parent)
    : QWidget(parent)
{
    const int pad = DialogStyle::px(40);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(pad, pad, pad, pad);
    layout->setSpacing(DialogStyle::px(14));
    layout->addStretch(1);

    // The product mark, at the size the Info dialog uses. A locked terminal is
    // the screen a site sees most of, so it should look like the product
    // rather than like an error.
    m_mark = new QLabel(this);
    m_mark->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_mark);

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

    // The field and its reveal button, side by side.
    //
    // The button is a widget rather than a QLineEdit action: an action inside
    // the field renders at the style's small-icon size and sits hard against
    // the border, which on a 19" terminal is a target nobody can hit and an
    // icon nobody can see. Beside the field it can be the size of a finger.
    m_keyRow = new QWidget(this);
    QHBoxLayout* keyLayout = new QHBoxLayout(m_keyRow);
    keyLayout->setContentsMargins(0, 0, 0, 0);
    keyLayout->setSpacing(DialogStyle::px(12));

    m_key = new QLineEdit(m_keyRow);
    m_key->setAlignment(Qt::AlignCenter);
    m_key->setEchoMode(QLineEdit::Password);
    m_key->setMaxLength(64);
    connect(m_key, &QLineEdit::returnPressed, this, &LockScreen::onSubmit);
    keyLayout->addWidget(m_key);

    // Show what was typed.
    //
    // A worker code is long, keyed on a virtual keyboard, in a packhouse, and
    // hidden behind dots the whole way. Getting it wrong fails as "Worker not
    // found", which reads as a card problem rather than a typo - so the only
    // way to tell those apart is to be able to look.
    m_reveal = new QPushButton(m_keyRow);
    m_reveal->setObjectName("reveal");
    m_reveal->setCheckable(true);
    m_reveal->setFocusPolicy(Qt::NoFocus);
    const int eyeSize = DialogStyle::px(34);
    m_reveal->setIcon(tinted(":/images/eye.svg", QColor("#d9d9d9"), eyeSize));
    m_reveal->setIconSize(QSize(eyeSize, eyeSize));
    m_reveal->setFixedSize(DialogStyle::px(62), DialogStyle::px(62));
    m_reveal->setToolTip(tr("Show code"));
    connect(m_reveal, &QPushButton::toggled, this, [this](bool shown) {
        m_key->setEchoMode(shown ? QLineEdit::Normal : QLineEdit::Password);
        // Orange while the code is showing, so the state reads at a glance
        // from the icon as well as the border.
        const int size = DialogStyle::px(34);
        m_reveal->setIcon(shown ? tinted(":/images/eye-off.svg", QColor("#ff9800"), size)
                                : tinted(":/images/eye.svg", QColor("#d9d9d9"), size));
        m_reveal->setToolTip(shown ? tr("Hide code") : tr("Show code"));
    });
    keyLayout->addWidget(m_reveal);

    layout->addWidget(m_keyRow, 0, Qt::AlignCenter);

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
    applyMetrics();
    setMethod(Keypad);
}

void LockScreen::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    applyMetrics();
}

void LockScreen::applyMetrics()
{
    const int available = height();
    if (available <= 0 || available == m_metricsFor)
        return;   // nothing has changed; restyling on every paint is wasteful

    m_metricsFor = available;

    // 720 is the height this was laid out for. A shorter panel gets everything
    // in proportion rather than the same layout with its bottom cut off; the
    // floor stops a very short panel producing text nobody can read, at which
    // point the mark is dropped instead - it is decoration, and the field is
    // not.
    const qreal fit = qBound(qreal(0.55), available / qreal(720), qreal(1.0));

    const bool showMark = available >= 420;
    m_mark->setVisible(showMark);
    if (showMark)
        m_mark->setPixmap(RobotHead::pixmap(int(DialogStyle::px(72) * fit),
                                            RobotHead::Standard));

    const int pad = int(DialogStyle::px(40) * fit);
    layout()->setContentsMargins(pad, pad, pad, pad);
    layout()->setSpacing(int(DialogStyle::px(14) * fit));

    const int eye = int(DialogStyle::px(34) * fit);
    m_reveal->setIconSize(QSize(eye, eye));
    const int button = int(DialogStyle::px(62) * fit);
    m_reveal->setFixedSize(button, button);

    applyStyle(fit);
}

void LockScreen::applyStyle(qreal fit)
{
    // Black, like the view behind it, so locking does not flash a different
    // colour at somebody walking past.
    setStyleSheet(QString(
        "LockScreen { background: black; }"
        "QLabel { color: #d9d9d9; font-size: %1px; }"
        "QLabel#station { color: #ffffff; font-size: %2px; font-weight: bold; }"
        "QLabel#message { color: #ff9800; font-size: %3px; }"
        "QLineEdit { background: #1a1a1a; color: #ffffff; border: %4px solid #4d4d4d;"
        "  border-radius: %5px; padding: %6px %10px; font-size: %7px;"
        "  min-width: %8px; }"
        "QLineEdit:focus { border-color: #ff9800; }"
        "QPushButton { background: #4d4d4d; color: #ffffff; border: none;"
        "  border-radius: %5px; padding: %6px %9px; font-size: %3px; }"
        "QPushButton:checked { background: #ff9800; color: #1a1a1a; }"
        // Showing the code is a state, not a press: the button stays dark
        // like the field it belongs to and marks itself with the accent
        // border. Filling it grey read as disabled, which is the opposite of
        // what it means.
        "QPushButton#reveal { background: #1a1a1a; border: %4px solid #4d4d4d; padding: 0; }"
        "QPushButton#reveal:checked { background: #1a1a1a; border-color: #ff9800; }"
        "QPushButton#reveal:pressed { background: #2a2a2a; }"
        "QPushButton:pressed { background: #ff9800; color: #1a1a1a; }")
        .arg(int(DialogStyle::px(20) * fit))
        .arg(int(DialogStyle::px(26) * fit))
        .arg(int(DialogStyle::px(18) * fit))
        .arg(DialogStyle::px(2))
        .arg(DialogStyle::px(6))
        .arg(int(DialogStyle::px(16) * fit))
        .arg(int(DialogStyle::px(34) * fit))
        .arg(int(DialogStyle::px(560) * fit))
        .arg(int(DialogStyle::px(24) * fit))
        // Room down the sides so a long code does not run under the eye, and
        // so the field does not sit hard against the screen edge.
        .arg(int(DialogStyle::px(28) * fit)));
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
    m_keyRow->setVisible(typing);
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

void LockScreen::focusKey()
{
    // Typing implies the keypad: an operator who reaches for the keyboard has
    // told us which method they want.
    if (m_method != Keypad)
        setMethod(Keypad);

    m_key->setFocus(Qt::OtherFocusReason);
}

void LockScreen::reset()
{
    clearMessage();
    m_key->clear();
    if (m_method == Keypad)
        m_key->setFocus();
}

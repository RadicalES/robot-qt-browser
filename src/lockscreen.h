#ifndef LOCKSCREEN_H
#define LOCKSCREEN_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>

class QAction;
class QVBoxLayout;
class QHBoxLayout;

// The sign-on screen, shown in place of the page when the terminal is locked.
//
// It covers the page and not the toolbar. WiFi, LAN, SCADA state and Info stay
// reachable without signing on, because a terminal whose network has died has
// to be fixable at the terminal, and none of those screens show a customer's
// data. What is behind the lock is the transaction page, and that is what
// matters.
//
// It lives in the central layout beside the web view rather than in a window
// of its own. The virtual keyboard binds to the *active window*, so a lock
// screen in its own window would take the keyboard with it and leave the
// operator unable to type — the same constraint that governs every dialog
// here. See refocusPage() in main.cpp.
//
// A worker is identified by a single value. It arrives keyed on the pad, from
// a card held to the reader, or from a scanned badge, and those are three ways
// of delivering the same digits, not three kinds of login.
class LockScreen : public QWidget {
    Q_OBJECT
public:
    enum Method {
        Keypad,     // typed
        Card,       // RFID card or scanned badge, through the reader bridge
    };

    explicit LockScreen(QWidget* parent = nullptr);

    // The station name, so an operator can see which terminal they are signing
    // on to. Sites run rows of identical terminals.
    void setStation(const QString& station);

    // Whether a reader is actually connected. A method that cannot work must
    // not be offered: an operator tapping "Card" on a terminal with no reader
    // learns nothing about why nothing happens.
    void setCardAvailable(bool available);

    // Shown after a refused sign-on. The server sends its own wording in the
    // response, and that is what an operator should be told — not a phrase
    // invented here that the person they call has never heard.
    void showError(const QString& message);
    void showBusy(const QString& message);
    void clearMessage();

    // Called when a card is presented while this screen is showing.
    void submitKey(const QString& key);

    // Focus and clear, ready for the next worker.
    void reset();

    // Put the caret in the code field.
    //
    // The virtual keyboard raises itself for whatever holds focus, so anything
    // that brings the keyboard up while the terminal is locked has to point it
    // here - otherwise the panel appears and the keystrokes go to the page
    // behind the lock, which is hidden and which nobody has signed on to see.
    void focusKey();

signals:
    // A key the operator wants to sign on with. Whether it is accepted is not
    // this widget's business — it asks the server (see the Robot API's
    // publishLogon) and is told showError() or nothing.
    void keyEntered(const QString& key);

private slots:
    void onSubmit();
    void onMethodChanged();

private:
    void applyStyle();
    void setMethod(Method method);

    QLabel* m_station;
    QLabel* m_prompt;
    QLabel* m_message;
    QLineEdit* m_key;
    QPushButton* m_reveal;
    QWidget* m_keyRow;
    QPushButton* m_signOn;
    QPushButton* m_keypadButton;
    QPushButton* m_cardButton;
    QWidget* m_entryRow;
    Method m_method = Keypad;
    bool m_cardAvailable = false;
};

#endif

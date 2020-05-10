#ifndef EVENTCONTROLLER_H
#define EVENTCONTROLLER_H

#include <QObject>

class EventController : public QObject
{
    Q_OBJECT
public:
    explicit EventController(QObject *parent = nullptr);
    void setKeyboardMode(int mode);

signals:
    void onKeyboardShown(int height);
    void onKeyboardHidden(void);
    void onKeyboardShowRequest(void);
    void onKeyboardHideRequest(void);
    void onSetKeyboardEnabledRequested(bool enabled);
    void customEvent(QEvent *event) override;
    void setInputMode(int mode);

public slots:
    void showKeyboard(void);
    void hideKeyboard(void);
    void setKeyboardEnabled(bool enabled);
    void setHeight(int height);

};

#endif // EVENTCONTROLLER_H

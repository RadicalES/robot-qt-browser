#include "eventcontroller.h"

EventController::EventController(QObject *parent) : QObject(parent)
{

}

void EventController::showKeyboard(void)
{
    emit onKeyboardShowRequest();
}

void EventController::hideKeyboard(void)
{
    emit onKeyboardHideRequest();
}

void EventController::setKeyboardEnabled(bool enabled)
{
    emit onSetKeyboardEnabledRequested(enabled);
}

void EventController::setKeyboardMode(int mode)
{
    emit setInputMode(mode);
}

void EventController::setHeight(int height)
{
    emit onKeyboardShown(height);
}

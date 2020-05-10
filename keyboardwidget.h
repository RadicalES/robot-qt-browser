#ifndef KEYBOARDWIDGET_H
#define KEYBOARDWIDGET_H

#include <QQuickWidget>

class KeyboardWidget : public QQuickWidget
{
public:
    KeyboardWidget(QWidget *parent);

protected:
    void mousePressEvent(QMouseEvent *event);
};

#endif // KEYBOARDWIDGET_H

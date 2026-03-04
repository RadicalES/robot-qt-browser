#ifndef DIGITALCLOCK_H
#define DIGITALCLOCK_H

#include <QLabel>
#include <QTimer>
#include <QTime>

class DigitalClock : public QLabel {
    Q_OBJECT
public:
    explicit DigitalClock(QWidget* parent = nullptr)
        : QLabel(parent)
        , m_colonVisible(true)
    {
        setStyleSheet("color: white; font-family: monospace; font-size: 16px;");
        setAlignment(Qt::AlignCenter);
        setFixedWidth(50);
        updateDisplay();

        connect(&m_timer, &QTimer::timeout, this, &DigitalClock::tick);
        m_timer.start(1000);
    }

private slots:
    void tick()
    {
        m_colonVisible = !m_colonVisible;
        updateDisplay();
    }

private:
    void updateDisplay()
    {
        QTime now = QTime::currentTime();
        QString sep = m_colonVisible ? ":" : " ";
        setText(now.toString("hh") + sep + now.toString("mm"));
    }

    QTimer m_timer;
    bool m_colonVisible;
};

#endif

#ifndef DIGITALCLOCK_H
#define DIGITALCLOCK_H

#include <QLabel>
#include <QString>
#include <QTimer>
#include <QTime>

class DigitalClock : public QLabel {
    Q_OBJECT
public:
    explicit DigitalClock(QWidget* parent = nullptr)
        : QLabel(parent)
        , m_colonVisible(true)
    {
        setFontPixels(24);
        setAlignment(Qt::AlignCenter);
        updateDisplay();

        connect(&m_timer, &QTimer::timeout, this, &DigitalClock::tick);
        m_timer.start(1000);
    }

    // Scales with the rest of the toolbar when a device profile is drawn
    // smaller than 1:1.
    void setFontPixels(int pixels)
    {
        setStyleSheet(QString("color: #d9d9d9; font-family: monospace; "
                              "font-size: %1px;").arg(pixels));
        // The width is the font's: 78px held "00:00" at 24px with room for the
        // colon to blink without the digits moving.
        setFixedWidth(pixels * 78 / 24);
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

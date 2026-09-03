#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <functional>
#include "robothead.h"

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class SystemController;

class InfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit InfoDialog(SystemController* sysCtrl, QWidget* parent = nullptr);

    // Reboot and Reset Defaults act on the machine this is running on. That is
    // the terminal in a kiosk, and somebody's workstation when the browser is
    // reproducing a terminal on a PC — where neither belongs.
    void setSystemActionsVisible(bool visible);

    // Adds a "Settings…" button that runs the given action. Only called where
    // the profile allows it, so on a terminal the button does not exist at all
    // rather than existing and refusing.
    void addSettingsButton(const std::function<void()>& open);

    // Adds the text-size row: smaller, the size now, larger.
    //
    // The page belongs to somebody else and is laid out for a desk. Whoever is
    // standing at the terminal is the only one who knows whether they can read
    // it, so they are the one who sets it - and it is here, behind Info,
    // rather than on the toolbar, because it is set once for a panel and a
    // page and then left alone.
    //
    // apply() shows the new size at once; the operator is looking at the page
    // while pressing, and a size that arrives after a dialog is closed is a
    // size nobody can judge.
    void addZoomRow(qreal current, const std::function<void(qreal)>& apply);

private:
    QLabel* m_head;
    QHBoxLayout* m_buttonRow;
    QPushButton* m_resetBtn;
    QPushButton* m_rebootBtn;
    QLabel* m_zoomLabel = nullptr;
    QVBoxLayout* m_layout = nullptr;
};

#endif

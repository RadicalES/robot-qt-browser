#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <functional>
#include "robothead.h"

class QLabel;
class QPushButton;
class QHBoxLayout;
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

private:
    QLabel* m_head;
    QHBoxLayout* m_buttonRow;
    QPushButton* m_resetBtn;
    QPushButton* m_rebootBtn;
};

#endif

#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include "robothead.h"

class QLabel;
class QPushButton;
class SystemController;

class InfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit InfoDialog(SystemController* sysCtrl, QWidget* parent = nullptr);

    // Recolours the mascot to the current SCADA state. Called before the dialog
    // is shown, so the head agrees with the toolbar indicator behind it.
    void setStatus(RobotHead::Variant variant);

    // Reboot and Reset Defaults act on the machine this is running on. That is
    // the terminal in a kiosk, and somebody's workstation when the browser is
    // reproducing a terminal on a PC — where neither belongs.
    void setSystemActionsVisible(bool visible);

private:
    QLabel* m_head;
    QPushButton* m_resetBtn;
    QPushButton* m_rebootBtn;
};

#endif

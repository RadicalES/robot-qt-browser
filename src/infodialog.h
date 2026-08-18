#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <QColor>

class QLabel;
class SystemController;

class InfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit InfoDialog(SystemController* sysCtrl, QWidget* parent = nullptr);

    // Recolours the mascot to the current SCADA state. Called before the dialog
    // is shown, so the head agrees with the toolbar indicator behind it.
    void setStatusColour(const QColor& colour);

private:
    QLabel* m_head;
};

#endif

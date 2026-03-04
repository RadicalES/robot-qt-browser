#ifndef REBOOTDIALOG_H
#define REBOOTDIALOG_H

#include <QDialog>

class SystemController;

class RebootDialog : public QDialog {
    Q_OBJECT
public:
    explicit RebootDialog(SystemController* sysCtrl, QWidget* parent = nullptr);
};

#endif

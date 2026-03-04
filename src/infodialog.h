#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>

class SystemController;

class InfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit InfoDialog(SystemController* sysCtrl, QWidget* parent = nullptr);
};

#endif

#include "rebootdialog.h"
#include "systemcontroller.h"
#include "dialogstyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

RebootDialog::RebootDialog(SystemController* sysCtrl, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Reboot");
    setStyleSheet("QDialog { background-color: #ffcccc; border: 2px solid black;"
                  "          border-radius: 6px; }" + DialogStyle::sheet());
    DialogStyle::widthToScreen(this, 0.88);

    auto* layout = new QVBoxLayout(this);

    auto* message = new QLabel("OK to restart?\nYour device will restart safely.");
    message->setStyleSheet("font-size: 24px; border: none;");
    message->setAlignment(Qt::AlignCenter);
    layout->addWidget(message);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch();

    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("background: white; border: 1px solid #999;");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonRow->addWidget(cancelBtn);

    auto* rebootBtn = new QPushButton("Reboot");
    rebootBtn->setStyleSheet("background: #ffa726; color: white;");
    connect(rebootBtn, &QPushButton::clicked, [this, sysCtrl]() {
        sysCtrl->reboot();
        accept();
    });
    buttonRow->addWidget(rebootBtn);

    layout->addLayout(buttonRow);
}

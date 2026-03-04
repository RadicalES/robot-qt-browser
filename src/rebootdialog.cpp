#include "rebootdialog.h"
#include "systemcontroller.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

RebootDialog::RebootDialog(SystemController* sysCtrl, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Reboot");
    setFixedSize(280, 140);
    setStyleSheet("background-color: #ffcccc; border: 2px solid black; border-radius: 4px;");

    auto* layout = new QVBoxLayout(this);

    auto* message = new QLabel("OK to restart?\nYour device will restart safely.");
    message->setStyleSheet("font-size: 14px; border: none;");
    message->setAlignment(Qt::AlignCenter);
    layout->addWidget(message);

    layout->addStretch();

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch();

    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("font-size: 13px; padding: 6px 16px; border: 1px solid #999; border-radius: 3px; background: white;");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonRow->addWidget(cancelBtn);

    auto* rebootBtn = new QPushButton("Reboot");
    rebootBtn->setStyleSheet("font-size: 13px; padding: 6px 16px; border-radius: 3px; background: #ffa726; color: white; border: none;");
    connect(rebootBtn, &QPushButton::clicked, [this, sysCtrl]() {
        sysCtrl->reboot();
        accept();
    });
    buttonRow->addWidget(rebootBtn);

    layout->addLayout(buttonRow);
}

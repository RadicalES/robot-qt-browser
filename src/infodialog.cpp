#include "infodialog.h"
#include "rebootdialog.h"
#include "systemcontroller.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

InfoDialog::InfoDialog(SystemController* sysCtrl, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("System Info");
    setStyleSheet("background-color: #f0f0f0;");

    auto* layout = new QVBoxLayout(this);

    // Header
    auto* header = new QLabel("System Info");
    header->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(header);

    // System info text
    auto* infoText = new QLabel(sysCtrl->systemInfo());
    infoText->setStyleSheet(
        "font-family: monospace; font-size: 13px; background: white; "
        "border: 1px solid #ccc; border-radius: 4px; padding: 8px;");
    infoText->setWordWrap(true);
    infoText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(infoText);

    layout->addStretch();

    // Action buttons
    auto* buttonRow = new QHBoxLayout;

    auto* resetBtn = new QPushButton("Reset Defaults");
    resetBtn->setStyleSheet(
        "font-size: 13px; padding: 6px 12px; border-radius: 3px; "
        "background: #ef5350; color: white; border: none;");
    connect(resetBtn, &QPushButton::clicked, [sysCtrl]() {
        sysCtrl->resetDefaults();
    });
    buttonRow->addWidget(resetBtn);

    buttonRow->addStretch();

    auto* rebootBtn = new QPushButton("Reboot");
    rebootBtn->setStyleSheet(
        "font-size: 13px; padding: 6px 12px; border-radius: 3px; "
        "background: #ffa726; color: white; border: none;");
    connect(rebootBtn, &QPushButton::clicked, [this, sysCtrl]() {
        RebootDialog dlg(sysCtrl, this);
        dlg.exec();
    });
    buttonRow->addWidget(rebootBtn);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "font-size: 13px; padding: 6px 12px; border-radius: 3px; "
        "background: #42a5f5; color: white; border: none;");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonRow->addWidget(closeBtn);

    layout->addLayout(buttonRow);
}

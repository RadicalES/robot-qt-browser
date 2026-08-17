#include "infodialog.h"
#include "rebootdialog.h"
#include "systemcontroller.h"
#include "dialogstyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

InfoDialog::InfoDialog(SystemController* sysCtrl, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("System Info");
    setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());
    DialogStyle::widthToScreen(this, 0.94);

    auto* layout = new QVBoxLayout(this);

    // Header
    auto* header = new QLabel("System Info");
    header->setStyleSheet("font-size: 30px; font-weight: bold;");
    layout->addWidget(header);

    // System info text
    auto* infoText = new QLabel(sysCtrl->systemInfo());
    infoText->setStyleSheet(
        "font-family: monospace; font-size: 19px; background: white; "
        "border: 1px solid #ccc; border-radius: 6px; padding: 14px;");
    infoText->setWordWrap(true);
    infoText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(infoText);

    // Action buttons
    auto* buttonRow = new QHBoxLayout;

    auto* resetBtn = new QPushButton("Reset Defaults");
    resetBtn->setStyleSheet(
        "background: #ef5350; color: white;");
    connect(resetBtn, &QPushButton::clicked, [sysCtrl]() {
        sysCtrl->resetDefaults();
    });
    buttonRow->addWidget(resetBtn);

    buttonRow->addStretch();

    auto* rebootBtn = new QPushButton("Reboot");
    rebootBtn->setStyleSheet(
        "background: #ffa726; color: white;");
    connect(rebootBtn, &QPushButton::clicked, [this, sysCtrl]() {
        RebootDialog dlg(sysCtrl, this);
        dlg.exec();
    });
    buttonRow->addWidget(rebootBtn);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "background: #42a5f5; color: white;");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonRow->addWidget(closeBtn);

    layout->addLayout(buttonRow);
}

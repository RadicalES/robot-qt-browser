#include "settingsdialog.h"
#include <QSpinBox>
#include "securitypolicy.h"
#include "dialogstyle.h"
#include "appconfig.h"
#include "runprofile.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

SettingsDialog::SettingsDialog(const QString& remoteUrl,
                               const QString& localUrl,
                               const QString& profileName,
                               int lockMinutes,
                               QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setStyleSheet("QDialog { background-color: #f0f0f0; }" + DialogStyle::sheet());
    DialogStyle::widthToScreen(this, 0.94);

    m_layout = new QVBoxLayout(this);

    auto* header = new QLabel("Settings");
    header->setStyleSheet(QString("font-size: %1px; font-weight: bold;")
                              .arg(DialogStyle::px(28)));
    m_layout->addWidget(header);

    // --- Device ----------------------------------------------------------
    // Which terminal to imitate. The whole point of the developer build is
    // seeing a page at the device's own viewport, and switching device should
    // not mean going back to a terminal to retype a command line.
    QVBoxLayout* device = addSection("Device");

    device->addWidget(new QLabel("Reproduce this terminal — its screen size, "
                                 "its network controls and its keyboard"));
    m_profile = new QComboBox;
    for (const QString& name : RunProfile::names()) {
        if (name == "kiosk")
            continue;   // that is a terminal being itself, not a PC imitating one
        m_profile->addItem(RunProfile::describe(name), name);
    }
    const int current = m_profile->findData(profileName);
    m_profile->setCurrentIndex(current >= 0 ? current : 0);
    device->addWidget(m_profile);

    // --- Pages -----------------------------------------------------------
    // Labelled as the toolbar buttons are, not as the config keys are:
    // whoever opens this has just used the toolbar and has no reason to know
    // about WB_REMOTE_URL.
    QVBoxLayout* pages = addSection("Pages");

    pages->addWidget(new QLabel("Remote — the transaction page, and the page "
                                "the browser opens with"));
    m_remote = new QLineEdit(remoteUrl);
    m_remote->setPlaceholderText("https://…");
    pages->addWidget(m_remote);

    pages->addWidget(new QLabel("Home — the local web UI"));
    m_local = new QLineEdit(localUrl);
    m_local->setPlaceholderText("http://localhost:3000");
    pages->addWidget(m_local);

    // --- Security ---------------------------------------------------------
    // Whether this terminal locks at all is not set here. That is the site's
    // decision, taken on the SCADA server as "Secure Terminal" and delivered
    // in the setup it publishes — a terminal that could turn its own lock off
    // would not be a secure terminal. The only thing left to say is how long
    // it waits, and the server will send that too in time.
    QVBoxLayout* security = addSection("Security");

    security->addWidget(new QLabel("On a secure terminal, how long before it "
                                   "locks itself and signs the worker off"));

    QWidget* minutesRow = new QWidget;
    QHBoxLayout* minutesLayout = new QHBoxLayout(minutesRow);
    minutesLayout->setContentsMargins(0, 0, 0, 0);
    minutesLayout->addWidget(new QLabel("Lock after"));

    m_lockMinutes = new QSpinBox;
    // One minute is the floor: zero would mean locking the instant somebody
    // stops touching it, which is a terminal nobody can use. Two hours is the
    // ceiling for the same reason in the other direction.
    m_lockMinutes->setRange(1, 120);
    m_lockMinutes->setValue(lockMinutes > 0 ? lockMinutes
                                            : SecurityPolicy::kDefaultIdleMinutes);
    m_lockMinutes->setSuffix(" minutes");
    minutesLayout->addWidget(m_lockMinutes);
    minutesLayout->addStretch(1);
    security->addWidget(minutesRow);

    // --- Footer ----------------------------------------------------------
    m_error = new QLabel;
    m_error->setStyleSheet(QString("color: #c62828; font-size: %1px;")
                               .arg(DialogStyle::px(18)));
    m_error->setWordWrap(true);
    m_error->hide();
    m_layout->addWidget(m_error);

    auto* note = new QLabel("Saved for this user in " + savePath());
    note->setStyleSheet(QString("color: #666; font-size: %1px;")
                            .arg(DialogStyle::px(16)));
    note->setWordWrap(true);
    m_layout->addWidget(note);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch();

    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet(DialogStyle::Colour::neutral());
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonRow->addWidget(cancelBtn);

    auto* saveBtn = new QPushButton("Save");
    saveBtn->setStyleSheet(DialogStyle::Colour::primary());
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    buttonRow->addWidget(saveBtn);

    m_layout->addLayout(buttonRow);

    // Text fields are the exception to the kiosk focus rule — they are what
    // the keyboard types into, on a terminal that has one.
    DialogStyle::takeNoFocusExceptFields(this);
}

QVBoxLayout* SettingsDialog::addSection(const QString& title)
{
    auto* heading = new QLabel(title);
    heading->setStyleSheet(QString("font-size: %1px; font-weight: bold; "
                                   "color: #444; margin-top: %2px;")
                               .arg(DialogStyle::px(20))
                               .arg(DialogStyle::px(8)));
    m_layout->addWidget(heading);

    auto* section = new QVBoxLayout;
    m_layout->addLayout(section);
    return section;
}

QString SettingsDialog::profileName() const
{
    return m_profile->currentData().toString();
}

int SettingsDialog::lockMinutes() const { return m_lockMinutes->value(); }

QString SettingsDialog::remoteUrl() const { return m_remote->text().trimmed(); }
QString SettingsDialog::localUrl() const  { return m_local->text().trimmed(); }

void SettingsDialog::onAccept()
{
    // A URL with no scheme loads as a relative path and fails in a way that
    // looks like the page is broken rather than the setting, so it is caught
    // here instead.
    const QString remote = remoteUrl();
    const QString local = localUrl();
    for (const QString& url : {remote, local}) {
        if (url.isEmpty())
            continue;
        const QUrl parsed(url);
        if (!parsed.isValid() || parsed.scheme().isEmpty()) {
            m_error->setText("\"" + url + "\" needs a scheme — http:// or https://");
            m_error->show();
            return;
        }
    }

    QString error;
    if (!save(remote, local, profileName(), lockMinutes(), &error)) {
        m_error->setText(error);
        m_error->show();
        return;
    }
    accept();
}

QString SettingsDialog::savePath()
{
    // Per user, not /etc: a desktop install's config file is root-owned and
    // shared by everyone on the machine, and one developer's dev server is not
    // a machine-wide setting.
    //
    // ~/.config/robot-browser/browser.config — the package's name and the same
    // file name as the system config, rather than AppConfigLocation's
    // "Radical Electronic Systems/RobotBrowser", which is a mouthful to type
    // and to print in this dialog.
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation);
    return dir + "/robot-browser/browser.config";
}

bool SettingsDialog::save(const QString& remoteUrl, const QString& localUrl,
                          const QString& profileName, int lockMinutes,
                          QString* error)
{
    const QString path = savePath();
    const QString dir = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(dir)) {
        if (error) *error = "Cannot create " + dir;
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) *error = "Cannot write " + path + ": " + file.errorString();
        return false;
    }

    // The same keys the system config uses, so this file is read by exactly
    // the same parser and either can be edited by hand.
    QTextStream out(&file);
    out << "# robot-browser, per-user settings. Written by the Settings\n"
        << "# dialog; delete this file to go back to the system settings in\n"
        << "# " << AppConfig::defaultPath() << ".\n"
        << "WB_REMOTE_URL=" << remoteUrl << "\n"
        << "WB_LOCAL_URL=" << localUrl << "\n"
        << "WB_PROFILE=" << profileName << "\n"
        << "WB_LOCK_MINUTES=" << lockMinutes << "\n";
    file.close();
    return true;
}

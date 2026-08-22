#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QLabel;
class QVBoxLayout;
class QComboBox;

// Settings a person can change while the browser is running.
//
// Only where a profile allows it, which means on a PC. A terminal is told what
// it is by its config file and by whatever provisioning layer owns it, and a
// kiosk whose settings an operator can change from the toolbar is not a kiosk.
// On a developer's machine the opposite holds: pointing the browser at a dev
// server is the entire workflow, and restarting it with two arguments to do
// that gets old.
//
// One dialog for everything of this kind, not one per setting. Today it holds
// the two page URLs; add a section rather than another dialog.
//
// Everything here is saved per user, not into /etc: a desktop install's config
// file is root-owned and shared by everyone on the machine.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    // Values are passed in and read back out rather than written directly, so
    // this dialog knows nothing about who owns them.
    SettingsDialog(const QString& remoteUrl, const QString& localUrl,
                   const QString& profileName, QWidget* parent = nullptr);

    QString remoteUrl() const;
    QString localUrl() const;

    // The device profile chosen in the dialog, or empty if it was left alone.
    // Applying it means restarting: geometry, chrome and keyboard layout are
    // all settled at startup.
    QString profileName() const;

    // Where settings are saved, shown in the dialog so it is obvious what this
    // changes and what to delete to undo it.
    static QString savePath();

    // Saves for the next run. False with a reason if the file cannot be
    // written.
    static bool save(const QString& remoteUrl, const QString& localUrl,
                     const QString& profileName, QString* error = nullptr);

private slots:
    void onAccept();

private:
    // Section header plus its own column, so a new group of settings is a few
    // lines rather than a rearrangement.
    QVBoxLayout* addSection(const QString& title);

    QVBoxLayout* m_layout;
    QComboBox* m_profile;
    QLineEdit* m_remote;
    QLineEdit* m_local;
    QLabel* m_error;
};

#endif

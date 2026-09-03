#include "pagezoom.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#include "appconfig.h"

namespace {

// What the buttons step through. Coarse enough that one press is worth
// pressing, and stopping at the ends rather than wrapping - an operator
// holding the button down should arrive somewhere and stay there.
const qreal kSteps[] = { 0.75, 0.9, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0 };
const int kStepCount = int(sizeof(kSteps) / sizeof(kSteps[0]));

}  // namespace

QString PageZoom::path()
{
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation);
    return dir + "/robot-browser/zoom";
}

qreal PageZoom::saved()
{
    QFile file(path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0.0;

    QTextStream in(&file);
    bool ok = false;
    const qreal zoom = in.readLine().trimmed().toDouble(&ok);
    if (!ok || zoom < AppConfig::kMinZoom || zoom > AppConfig::kMaxZoom)
        return 0.0;
    return zoom;
}

bool PageZoom::save(qreal zoom)
{
    const QString file_path = path();
    if (!QDir().mkpath(QFileInfo(file_path).absolutePath()))
        return false;

    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    QTextStream out(&file);
    out << QString::number(zoom, 'f', 2) << "\n";
    return true;
}

bool PageZoom::clear()
{
    QFile file(path());
    return !file.exists() || file.remove();
}

qreal PageZoom::stepUp(qreal from)
{
    for (int i = 0; i < kStepCount; ++i) {
        if (kSteps[i] > from + 0.001)
            return kSteps[i];
    }
    return kSteps[kStepCount - 1];
}

qreal PageZoom::stepDown(qreal from)
{
    for (int i = kStepCount - 1; i >= 0; --i) {
        if (kSteps[i] < from - 0.001)
            return kSteps[i];
    }
    return kSteps[0];
}

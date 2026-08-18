// Phase 0 spike — does QtWebEngine run acceptably on CM4 (2GB, VideoCore VI)?
//
// Throwaway. Not part of robot-browser. Measures the three things that decide
// go/no-go: time to first paint, whether the GPU is actually used (as opposed
// to SwiftShader software rendering), and frame pacing under a scroll load.
// Memory is measured externally by measure.sh — Chromium is multi-process, so
// this process's own RSS is meaningless on its own.
//
// Build and run on the target, not cross-compiled — see README.md.

#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <QScreen>
#include <QCommandLineParser>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QDebug>

// Page subclass that forwards console output to stdout. The real app forwards
// it to WebsockServer; here we just want the SPIKE_* markers on the terminal.
class SpikePage : public QWebEnginePage {
public:
    using QWebEnginePage::QWebEnginePage;

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString& message,
                                  int lineNumber,
                                  const QString& sourceId) override
    {
        Q_UNUSED(level); Q_UNUSED(lineNumber); Q_UNUSED(sourceId);
        if (message.startsWith("SPIKE_"))
            qInfo().noquote() << message;
    }
};

// Reports the real GL renderer string. "V3D" or "VideoCore" means the GPU is
// live; "SwiftShader" or "llvmpipe" means Chromium fell back to software
// rendering, which on a CM4 is an immediate no-go.
static const char* kGlProbe = R"JS(
(function () {
    var c = document.createElement('canvas');
    var gl = c.getContext('webgl') || c.getContext('experimental-webgl');
    if (!gl) { console.log('SPIKE_GL none (no WebGL context)'); return; }
    var dbg = gl.getExtension('WEBGL_debug_renderer_info');
    var r = dbg ? gl.getParameter(dbg.UNMASKED_RENDERER_WEBGL)
                : gl.getParameter(gl.RENDERER);
    console.log('SPIKE_GL ' + r);
})();
)JS";

// Scrolls the page for N ms under requestAnimationFrame and reports frame
// pacing. Average FPS hides stalls, so the worst frame time is reported too —
// a 400ms hitch inside an otherwise smooth run still fails a kiosk UI.
static QString scrollProbe(int durationMs)
{
    return QString(R"JS(
(function () {
    var frames = 0, worst = 0, first = true;
    var start = performance.now(), last = start;
    function step(now) {
        var dt = now - last;
        last = now;
        // Skip the first interval: it includes rAF scheduling, not paint cost.
        if (first) { first = false; } else { frames++; if (dt > worst) worst = dt; }
        window.scrollBy(0, 4);
        if (now - start < %1) {
            requestAnimationFrame(step);
        } else {
            var secs = (now - start) / 1000;
            console.log('SPIKE_FPS avg=' + (frames / secs).toFixed(1) +
                        ' worstFrame=' + worst.toFixed(1) + 'ms' +
                        ' frames=' + frames);
            console.log('SPIKE_DONE');
        }
    }
    requestAnimationFrame(step);
})();
)JS").arg(durationMs);
}

int main(int argc, char** argv)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt 5's QtWebEngine requires this before the QApplication is constructed.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("webengine-spike");

    QElapsedTimer uptime;
    uptime.start();

    QCommandLineParser parser;
    parser.setApplicationDescription("QtWebEngine feasibility spike for CM4");
    parser.addHelpOption();
    parser.addPositionalArgument("url", "URL to load (the real transaction URL)");
    QCommandLineOption scrollOpt({"s", "scroll"},
        "Scroll-test duration in milliseconds.", "ms", "8000");
    QCommandLineOption keepOpt({"k", "keep"},
        "Stay open after the run instead of exiting (for manual input feel).");
    parser.addOption(scrollOpt);
    parser.addOption(keepOpt);
    parser.process(app);

    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty()) {
        qCritical() << "usage: webengine-spike <url> [--scroll ms] [--keep]";
        return 2;
    }
    const QUrl url(positional.first());
    const int scrollMs = parser.value(scrollOpt).toInt();
    const bool keepOpen = parser.isSet(keepOpt);

    qInfo().noquote() << "SPIKE_QT" << QT_VERSION_STR;
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    qInfo().noquote() << "SPIKE_CHROMIUM" << qWebEngineChromiumVersion();
#endif
    qInfo().noquote() << "SPIKE_PID" << QCoreApplication::applicationPid();
    qInfo().noquote() << "SPIKE_URL" << url.toString();

    // Persistent profile — the real app needs cookies to survive a reboot, and
    // profile setup is what replaces TestBrowserCookieJar in the port.
    QWebEngineProfile profile("robot-browser-spike");
    profile.setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    profile.settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    SpikePage page(&profile);
    QWebEngineView view;
    view.setPage(&page);

    QObject::connect(&page, &QWebEnginePage::loadFinished,
                     [&](bool ok) {
        qInfo().noquote() << "SPIKE_LOAD"
                          << (ok ? "ok" : "FAILED")
                          << uptime.elapsed() << "ms from process start";
        if (!ok) {
            qCritical() << "load failed — check the URL and network";
            app.exit(1);
            return;
        }
        page.runJavaScript(QString::fromLatin1(kGlProbe));
        page.runJavaScript(scrollProbe(scrollMs));

        if (!keepOpen) {
            // Margin over the scroll test so the final console line lands.
            QTimer::singleShot(scrollMs + 2000, &app, [&]() {
                qInfo().noquote() << "SPIKE_EXIT after" << uptime.elapsed() << "ms";
                app.quit();
            });
        }
    });

    // Fullscreen matches how the kiosk actually runs — windowed understates
    // the compositing cost at the panel's real resolution.
    QScreen* screen = app.primaryScreen();
    view.setGeometry(screen->geometry());
    view.showFullScreen();

    view.setUrl(url);
    return app.exec();
}

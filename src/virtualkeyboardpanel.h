#ifndef VIRTUALKEYBOARDPANEL_H
#define VIRTUALKEYBOARDPANEL_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QInputMethod>
#include <QMetaProperty>
#include <QDebug>

// Hosts QtQuick.VirtualKeyboard's InputPanel inside the widgets shell.
//
// Debian's Qt 6 QtVirtualKeyboard has no desktop integration compiled in, so
// nothing creates a keyboard window on its own: the input context calls
// showInputPanel() and the panel simply does not exist. The application has to
// host InputPanel itself — the same workaround the Qt 5 QML overlay used.
//
// Show/hide follows the QML root's keyboardActive property, which mirrors
// InputPanel.active. QInputMethod::visibleChanged looks like the obvious signal
// to use but does not track this panel, and keying off it left the keyboard
// failing to pop up when a web input field took focus.
class VirtualKeyboardPanel : public QQuickWidget {
    Q_OBJECT

public:
    explicit VirtualKeyboardPanel(QWidget* parent = nullptr)
        : QQuickWidget(parent)
    {
        // The custom "robot" keyboard style ships with the package rather than
        // with Qt, so its import root has to be added before the QML loads.
        // Harmless when the directory is absent — QtVirtualKeyboard then falls
        // back to its default style.
        engine()->addImportPath(QStringLiteral(ROBOT_BROWSER_QML_DIR));

        // The widget takes its size from the QML root, whose height tracks the
        // keyboard. Sizing the other way round would squash the key rows.
        setResizeMode(QQuickWidget::SizeViewToRootObject);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        // Never take Qt focus. A QQuickWidget grabs focus when clicked, which
        // would pull it off the web view being typed into — the input panel
        // then deactivates and the keyboard vanishes on the first keypress.
        setFocusPolicy(Qt::NoFocus);
        setClearColor(Qt::transparent);
        setSource(QUrl(QStringLiteral("qrc:/qml/virtualkeyboard.qml")));

        // Bind to the QML property's notify signal through the meta-object.
        // A SIGNAL("keyboardActiveChanged()") string does not match the signal
        // QML generates, so that connection silently fails and the keyboard
        // never appears.
        QQuickItem* root = rootObject();
        if (!root) {
            qWarning() << "VirtualKeyboardPanel: QML root failed to load —"
                       << "no on-screen keyboard";
            return;
        }
        const QMetaObject* rootMeta = root->metaObject();
        const int propIndex = rootMeta->indexOfProperty("keyboardActive");
        const int slotIndex =
            metaObject()->indexOfSlot("onKeyboardActiveChanged()");
        if (propIndex < 0 || slotIndex < 0
            || !rootMeta->property(propIndex).hasNotifySignal()) {
            qWarning() << "VirtualKeyboardPanel: cannot track keyboardActive —"
                       << "no on-screen keyboard";
            return;
        }
        connect(root, rootMeta->property(propIndex).notifySignal(),
                this, metaObject()->method(slotIndex));

        // Drive the widget height from the QML root explicitly.
        //
        // When the keyboard activates, its height is still 0 — the QML binding
        // has not evaluated yet — so relying on the size hint to propagate
        // afterwards is a race the layout sometimes loses, leaving the panel
        // collapsed even though the keyboard was asked for. Following
        // heightChanged and setting a fixed height re-runs the layout the
        // moment the real height is known.
        connect(root, SIGNAL(heightChanged()), this, SLOT(onRootHeightChanged()));
    }

    // Show the keyboard regardless of the input context, and hide it again.
    // Driven by the toolbar button: an operator who cannot get the keyboard up
    // has no other way out, and one who wants it gone while reading a page
    // should not have to find somewhere neutral to tap.
    void setForceVisible(bool force)
    {
        if (QQuickItem* root = rootObject())
            root->setProperty("forceVisible", force);
    }

    // Not isVisible(): the widget is always visible and collapses to zero
    // height instead, so widget visibility says nothing about the keyboard.
    bool isShowing() const
    {
        const QQuickItem* root = rootObject();
        return root && root->property("keyboardActive").toBool();
    }

    // Match the keyboard to the window width. InputPanel derives its height
    // from this, so the widget resizes to suit.
    void setPanelWidth(int width, int availableHeight = 0)
    {
        QQuickItem* root = rootObject();
        if (!root)
            return;

        // The keyboard's height follows its width — the QML sets
        // keyboardDesignHeight to 46.5% of the design width — which is right on
        // a portrait panel and wrong on a short landscape one. A T431 is
        // 1024x600, so a full-width keyboard wants 476 of those 600 pixels and
        // covers the page it is typing into.
        //
        // Narrow it rather than squash it. Squashing was the first attempt and
        // it made the letters enormous: the style sizes its text by scaleHint,
        // which is width / keyboardDesignWidth, so changing only the height
        // ratio shrinks the keys and leaves the font where it was. Narrowing
        // takes height, width and text down together, and the keyboard keeps
        // its proportions.
        int effectiveWidth = width;
        if (availableHeight > 0) {
            const int maxHeight = int(availableHeight * kMaxHeightFraction);
            const int widthForHeight = int(maxHeight / kAspect);
            if (widthForHeight < width) {
                // Narrowing is needed, so apply the width limit as well: a
                // third of the panel each way. Both caps belong to the same
                // case — a short landscape screen — and neither applies to a
                // portrait panel, where a full-width keyboard is already well
                // under a third of the height and a third of the width would
                // leave keys too small to hit.
                effectiveWidth = qMin(widthForHeight,
                                      int(width * kMaxWidthFraction));
            }
        }

        m_panelWidth = effectiveWidth;
        setFixedWidth(effectiveWidth);
        root->setWidth(effectiveWidth);

        if (qEnvironmentVariableIsSet("ROBOT_BROWSER_DEBUG_GEOMETRY")) {
            qWarning("keyboard: asked %dx? avail-h %d -> width %d, root %.0fx%.0f, widget %dx%d",
                     width, availableHeight, effectiveWidth,
                     root->width(), root->height(), this->width(), this->height());
        }
    }

    // Dock the keyboard down the side of the page rather than under it.
    //
    // The panel keeps its own width and takes the full height available; the
    // QML height ratio is set from that, so the keys grow vertically instead
    // of the keyboard growing wider. Used on short landscape panels, where a
    // full-width keyboard would leave a strip of page too small to read.
    void setSideDocked(int width, int height)
    {
        QQuickItem* root = rootObject();
        if (!root)
            return;

        m_sideDocked = true;
        m_panelWidth = width;
        setFixedWidth(width);
        // A maximum, not a fixed size: if the layout has less to give — a
        // banner is showing, or the window is smaller than the screen — the
        // keyboard has to shrink rather than force the window taller.
        setMaximumHeight(height);
        setMinimumHeight(0);

        // Fill the height: the whole point of docking to the side is that the
        // vertical room is there to use.
        if (width > 0)
            root->setProperty("heightRatio", qreal(height) / width);

        // Keep the letters the size they are on a full-width keyboard. The
        // style scales its text by width / keyboardDesignWidth, so a quarter
        // of the panel with the stock 2560 design width would give letters a
        // quarter the size. Measured on a T431: a 512px keyboard against 2560
        // reads well, which is a scale of 0.2.
        root->setProperty("designWidth", qreal(width) / kGlyphScale);
        root->setWidth(width);

        if (qEnvironmentVariableIsSet("ROBOT_BROWSER_DEBUG_GEOMETRY")) {
            qWarning("keyboard(side): %dx%d ratio %.2f designWidth %.0f",
                     width, height, qreal(height) / width, qreal(width) / kGlyphScale);
        }
    }

private:
    // A third of the screen, no more. The operator is typing into something and
    // has to be able to see it: on a 600px panel even half the screen leaves
    // too little page to work with.
    //
    // On a portrait panel the keyboard is well under this anyway — a T440's
    // 720-wide keyboard is 335 of 1280 — so this only bites on the short
    // landscape panels it is meant for.
    static constexpr qreal kMaxHeightFraction = 1.0 / 3.0;
    // The shape the QML asks for: keyboardDesignHeight = 46.5% of the width.
    static constexpr qreal kAspect = 0.465;
    static constexpr qreal kMaxWidthFraction = 1.0 / 3.0;
    // Glyphs read well at this scale — 512px of keyboard against the style's
    // 2560 design width, measured on a T431.
    static constexpr qreal kGlyphScale = 0.2;
    bool m_sideDocked = false;
    int m_panelWidth = 0;

private slots:
    void onRootHeightChanged()
    {
        // Side-docked, the layout owns the height — pinning it to the QML root
        // would collapse the column back to a bottom-strip shape.
        if (m_sideDocked)
            return;
        if (QQuickItem* root = rootObject())
            setFixedHeight(int(root->height()));
    }

    void onKeyboardActiveChanged()
    {
        QQuickItem* root = rootObject();
        const bool active = root && root->property("keyboardActive").toBool();
        if (active && root) {
            // Re-apply the width on every show, in case the window resized
            // while the keyboard was collapsed.
            //
            // The width we were GIVEN, not the parent's. Taking the parent's
            // undid the narrowing on a short landscape panel every time the
            // keyboard came up: it went back to full width, which is both half
            // the screen tall and — because the style scales its text by
            // width / keyboardDesignWidth — covered in enormous letters.
            root->setWidth(m_panelWidth > 0 ? m_panelWidth
                                            : (parentWidget() ? parentWidget()->width()
                                                              : int(root->width())));
        }

        if (qEnvironmentVariableIsSet("ROBOT_BROWSER_DEBUG_GEOMETRY") && root) {
            qWarning("keyboard(active=%d): root %.0fx%.0f widget %dx%d panelWidth %d",
                     active, root->width(), root->height(),
                     this->width(), this->height(), m_panelWidth);
        }
        qDebug() << "VirtualKeyboardPanel: active=" << active
                << "rootHeight=" << (root ? root->height() : -1)
                << "widgetHeight=" << height();

        if (!active) {
            // Resync the platform input context. When a dialog takes focus the
            // input panel deactivates and this widget hides, but the context is
            // never told — it still believes the panel is on screen, so the
            // next tap on a web input issues no showInputPanel() and the
            // keyboard never comes back. Telling it to hide restores the
            // transition. isVisible() guards against recursing through the
            // hideInputPanel this triggers.
            QInputMethod* inputMethod = QGuiApplication::inputMethod();
            if (inputMethod->isVisible())
                inputMethod->hide();
        }
    }
};

#endif

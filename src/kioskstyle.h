#ifndef KIOSKSTYLE_H
#define KIOSKSTYLE_H

#include <QProxyStyle>

// Always request the on-screen keyboard when an input is tapped.
//
// Qt decides whether a click should raise the software input panel from the
// SH_RequestSoftwareInputPanel style hint. The default,
// RSIP_OnMouseClickAndAlreadyFocused, only requests it when the widget clicked
// ALREADY had focus — so the first tap on a field merely focuses it and the
// keyboard appears only on a second tap. On a touch kiosk that reads as "the
// keyboard doesn't always pop up".
//
// RSIP_OnMouseClick requests it on every click, focused or not.
class KioskStyle : public QProxyStyle {
    Q_OBJECT

public:
    int styleHint(StyleHint hint,
                  const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr,
                  QStyleHintReturn* returnData = nullptr) const override
    {
        if (hint == SH_RequestSoftwareInputPanel)
            return RSIP_OnMouseClick;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

#endif

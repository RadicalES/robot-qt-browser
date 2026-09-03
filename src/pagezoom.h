#ifndef PAGEZOOM_H
#define PAGEZOOM_H

#include <QString>

// The zoom an operator chose at the terminal.
//
// Kept apart from browser.config on purpose. A terminal takes its
// configuration from the file its deployment controls and nothing else - which
// is why the Settings dialog does not exist on one, and why a user-level
// browser.config is read only where it does. That rule is about what a
// terminal IS: where it points, what it is called, whether it locks.
//
// How big the text is drawn is not that. It cannot point a terminal anywhere,
// cannot unlock it and cannot change what it does; it decides whether the
// person standing in front of it can read the screen. So it has its own small
// file, written by the terminal and read on every profile, and the deployment
// keeps the last word on everything else.
//
// Precedence: WB_PAGE_ZOOM in browser.config is the deployment's starting
// point; a zoom chosen here overrides it, on this terminal only.
namespace PageZoom {

// Where the choice is kept. ~/.config/robot-browser/zoom - beside the
// user-level browser.config the Settings dialog writes, so a machine has one
// place to look.
QString path();

// The saved zoom, or 0 when nothing has been chosen and the deployment's
// value should stand.
qreal saved();

// Remember a choice. Returns false when it could not be written, which is
// worth telling the operator: a zoom that forgets itself at the next reboot is
// worse than one that was never offered.
bool save(qreal zoom);

// Forget a choice, so the deployment's WB_PAGE_ZOOM stands again.
bool clear();

// The steps the buttons move through. Not a free number: an operator pressing
// a button wants a readable jump, and 5% steps mean twelve presses to get
// anywhere while every one of them redraws the page.
qreal stepUp(qreal from);
qreal stepDown(qreal from);

}  // namespace PageZoom

#endif

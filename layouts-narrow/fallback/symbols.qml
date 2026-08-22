// Narrow symbol pages — four keys per row, to match the narrow letter layout.
//
// The stock pages are ten across, which on a 256px side-docked keyboard gives
// 25px keys: the same problem the letters had, one page over. Same answer —
// fewer keys per row, more rows.
//
// Two pages, as the stock layout has: numbers first, because a terminal's
// operator is far more often typing a station number or an address than a
// currency symbol.
//
//   7 8 9 -          , . : ;
//   4 5 6 +          + - * /
//   1 2 3 *          @ _ ? !
//   0 , . /          ( ) " \\
//   ABC ⌫ ↵ SYM      # % & $
//                    = \ < >
//                    ABC 123 ⌫ ↵
//
// (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.VirtualKeyboard 2.1

KeyboardLayoutLoader {
    property bool secondPage: false

    onVisibleChanged: if (!visible) secondPage = false

    sourceComponent: secondPage ? symbolsPage : numbersPage

    // ---- numbers --------------------------------------------------------
    Component {
        id: numbersPage

        KeyboardLayout {
            keyWeight: 160

            // PC numpad order: 7-8-9 on top, arithmetic down the right, as on
            // every keyboard and calculator anyone entering figures has used.
            KeyboardRow {
                Key { key: Qt.Key_7; text: "7" }
                Key { key: Qt.Key_8; text: "8" }
                Key { key: Qt.Key_9; text: "9" }
                Key { key: Qt.Key_Minus; text: "-" }
            }
            KeyboardRow {
                Key { key: Qt.Key_4; text: "4" }
                Key { key: Qt.Key_5; text: "5" }
                Key { key: Qt.Key_6; text: "6" }
                Key { key: Qt.Key_Plus; text: "+" }
            }
            KeyboardRow {
                Key { key: Qt.Key_1; text: "1" }
                Key { key: Qt.Key_2; text: "2" }
                Key { key: Qt.Key_3; text: "3" }
                Key { key: Qt.Key_Asterisk; text: "*" }
            }
            KeyboardRow {
                Key { key: Qt.Key_0; text: "0" }
                Key { key: Qt.Key_Comma; text: "," }
                Key { key: Qt.Key_Period; text: "." }
                Key { key: Qt.Key_Slash; text: "/" }
            }
            // No space key: someone entering figures is not typing words, so
            // the slot carries the way to the punctuation page instead.
            KeyboardRow {
                SymbolModeKey { displayText: "ABC" }
                BackspaceKey {}
                EnterKey {}
                ModeKey {
                    displayText: "SYM"
                    onClicked: secondPage = true
                }
            }
        }
    }

    // ---- symbols --------------------------------------------------------
    Component {
        id: symbolsPage

        KeyboardLayout {
            keyWeight: 160

            // Punctuation that belongs together, together: the sentence marks
            // on one row, arithmetic on the next.
            KeyboardRow {
                Key { key: Qt.Key_Comma;     text: "," }
                Key { key: Qt.Key_Period;    text: "." }
                Key { key: Qt.Key_Colon;     text: ":" }
                Key { key: Qt.Key_Semicolon; text: ";" }
            }

            KeyboardRow {
                Key { key: Qt.Key_Plus;     text: "+" }
                Key { key: Qt.Key_Minus;    text: "-" }
                Key { key: Qt.Key_Asterisk; text: "*" }
                Key { key: Qt.Key_Slash;    text: "/" }
            }

            KeyboardRow {
                Key { key: Qt.Key_At;         text: "@" }
                Key { key: Qt.Key_Underscore; text: "_" }
                Key { key: Qt.Key_Question;   text: "?" }
                Key { key: Qt.Key_Exclam;     text: "!" }
            }

            KeyboardRow {
                Key { key: Qt.Key_ParenLeft;  text: "(" }
                Key { key: Qt.Key_ParenRight; text: ")" }
                Key { key: Qt.Key_QuoteDbl;   text: "\"" }
                // Backslash in the apostrophe's place: the double quote covers
                // quoting, and a keyboard that cannot type a path separator is
                // no use to someone entering one.
                Key { key: Qt.Key_Backslash;  text: "\\" }
            }

            KeyboardRow {
                Key { key: Qt.Key_NumberSign; text: "#" }
                Key { key: Qt.Key_Percent;    text: "%" }
                Key { key: Qt.Key_Ampersand;  text: "&" }
                Key { key: Qt.Key_Equal;      text: "=" }
            }

            KeyboardRow {
                SymbolModeKey { displayText: "ABC" }
                ModeKey {
                    displayText: "123"
                    onClicked: secondPage = false
                }
                BackspaceKey {}
                EnterKey {}
            }
        }
    }
}

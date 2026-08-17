/*
 * Numbers and symbols pages for the robot kiosk keyboard.
 *
 * Three keypads in total: letters (main.qml), numbers, symbols. The letters
 * page reaches numbers via its [123] key, numbers reaches symbols via [#+=],
 * and either returns to letters via [ABC].
 *
 * Rules for this keyboard:
 *  - one character per key; only the page-toggle keys carry a word or a
 *    symbol group as their label
 *  - the number pad uses four keys per row so the targets stay large
 *  - only the symbols production actually uses. The stock Qt page carried
 *    maths signs, fractions, four currencies and an emoticon.
 *
 * (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za
 */

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.VirtualKeyboard 2.1

KeyboardLayoutLoader {
    property bool secondPage

    onVisibleChanged: if (!visible) secondPage = false
    sourceComponent: secondPage ? symbolsPage : numbersPage

    // ---- page 2 of 3: numbers -------------------------------------------
    // Four keys per row rather than the stock ten, so each digit is a big
    // touch target.
    Component {
        id: numbersPage
        KeyboardLayout {
            keyWeight: 160

            KeyboardRow {
                Key { key: Qt.Key_1; text: "1" }
                Key { key: Qt.Key_2; text: "2" }
                Key { key: Qt.Key_3; text: "3" }
                BackspaceKey {}
            }
            KeyboardRow {
                Key { key: Qt.Key_4; text: "4" }
                Key { key: Qt.Key_5; text: "5" }
                Key { key: Qt.Key_6; text: "6" }
                EnterKey {}
            }
            KeyboardRow {
                Key { key: Qt.Key_7; text: "7" }
                Key { key: Qt.Key_8; text: "8" }
                Key { key: Qt.Key_9; text: "9" }
                ModeKey {
                    displayText: "#+="
                    onClicked: secondPage = true
                }
            }
            KeyboardRow {
                SymbolModeKey { displayText: "ABC" }
                Key { key: Qt.Key_0; text: "0" }
                Key { key: Qt.Key_Period; text: "." }
                SpaceKey {}
            }
        }
    }

    // ---- page 3 of 3: symbols -------------------------------------------
    Component {
        id: symbolsPage
        KeyboardLayout {
            keyWeight: 160

            KeyboardRow {
                Key { key: Qt.Key_At; text: "@" }
                Key { key: Qt.Key_NumberSign; text: "#" }
                Key { key: Qt.Key_Percent; text: "%" }
                Key { key: Qt.Key_Ampersand; text: "&" }
                Key { key: Qt.Key_Asterisk; text: "*" }
                BackspaceKey {}
            }
            KeyboardRow {
                Key { key: Qt.Key_Minus; text: "-" }
                Key { key: Qt.Key_Underscore; text: "_" }
                Key { key: Qt.Key_Plus; text: "+" }
                Key { key: Qt.Key_Equal; text: "=" }
                Key { key: Qt.Key_Slash; text: "/" }
                EnterKey {}
            }
            KeyboardRow {
                Key { key: Qt.Key_Colon; text: ":" }
                Key { key: Qt.Key_Semicolon; text: ";" }
                Key { key: Qt.Key_ParenLeft; text: "(" }
                Key { key: Qt.Key_ParenRight; text: ")" }
                Key { key: Qt.Key_Question; text: "?" }
                Key { key: Qt.Key_Exclam; text: "!" }
            }
            KeyboardRow {
                SymbolModeKey { displayText: "ABC" }
                ModeKey {
                    displayText: "123"
                    onClicked: secondPage = false
                }
                Key { key: Qt.Key_Apostrophe; text: "'" }
                Key { key: Qt.Key_Comma; text: "," }
                Key { key: Qt.Key_Period; text: "." }
                SpaceKey {}
            }
        }
    }
}

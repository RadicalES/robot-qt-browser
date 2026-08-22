// Full keyboard — letters, numpad and symbols side by side, in one keypad.
//
// For wide panels: a T432 is 1280x800 and an ITPC-200 is 1920x1080. On a
// screen that size there is no reason to hide two thirds of the keys behind a
// mode key, and no reason to leave half the width empty: everything an
// operator types is on one page, in three sections, with the keys they press
// constantly along the bottom.
//
//   q w e r t y u i o p     7 8 9     @ # $ %
//   a s d f g h j k l       4 5 6     - _ + =
//   z x c v b n m . , -     1 2 3     : ; ( )
//   ⇧ ␣ ⌫ ↵                 0 . ,     / \ ? !
//
// Three sections rather than three pages, because the room is there. The
// narrow layout in layouts-narrow/ exists for the opposite case, a 1024x600
// panel where nothing fits and pages are unavoidable.
//
// (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.VirtualKeyboard 2.1

KeyboardLayout {
    inputMode: InputEngine.InputMode.Latin
    keyWeight: 100

    KeyboardRow {

        // ---- letters ----------------------------------------------------
        KeyboardColumn {
            // 1160 rather than the 1000 the key count implies. The three
            // columns do not divide exactly as asked — measured on an
            // ITPC-200, the letters came out 62px per column against 72px for
            // the numbers and symbols — so the letters section is given the
            // difference back. Keys the same width across all three sections
            // is what matters; the numbers matching their key counts is not.
            Layout.preferredWidth: 1160

            KeyboardRow {
                Key { key: Qt.Key_Q; text: "q" }
                Key { key: Qt.Key_W; text: "w" }
                Key { key: Qt.Key_E; text: "e" }
                Key { key: Qt.Key_R; text: "r" }
                Key { key: Qt.Key_T; text: "t" }
                Key { key: Qt.Key_Y; text: "y" }
                Key { key: Qt.Key_U; text: "u" }
                Key { key: Qt.Key_I; text: "i" }
                Key { key: Qt.Key_O; text: "o" }
                Key { key: Qt.Key_P; text: "p" }
            }
            KeyboardRow {
                Key { key: Qt.Key_A; text: "a" }
                Key { key: Qt.Key_S; text: "s" }
                Key { key: Qt.Key_D; text: "d" }
                Key { key: Qt.Key_F; text: "f" }
                Key { key: Qt.Key_G; text: "g" }
                Key { key: Qt.Key_H; text: "h" }
                Key { key: Qt.Key_J; text: "j" }
                Key { key: Qt.Key_K; text: "k" }
                Key { key: Qt.Key_L; text: "l" }
                Key { key: Qt.Key_Apostrophe; text: "'"; alternativeKeys: "'\"" }
            }
            KeyboardRow {
                Key { key: Qt.Key_Z; text: "z" }
                Key { key: Qt.Key_X; text: "x" }
                Key { key: Qt.Key_C; text: "c" }
                Key { key: Qt.Key_V; text: "v" }
                Key { key: Qt.Key_B; text: "b" }
                Key { key: Qt.Key_N; text: "n" }
                Key { key: Qt.Key_M; text: "m" }
                Key { key: Qt.Key_Period; text: "."; alternativeKeys: ".>" }
                Key { key: Qt.Key_Comma;  text: ","; alternativeKeys: ",<" }
                Key { key: Qt.Key_Minus;  text: "-"; alternativeKeys: "-_" }
            }
            // Under the letters, where a typist expects them, and the same
            // size as every other key. A single full-width row across all
            // three sections made these enormous.
            KeyboardRow {
                ShiftKey {}
                SpaceKey { weight: 700 }
                BackspaceKey {}
                EnterKey {}
            }
        }

        // A gap between sections, so three groups read as three groups.
        FillerKey { weight: 40 }

        // ---- numbers, in numpad order -----------------------------------
        KeyboardColumn {
            Layout.preferredWidth: 300

            KeyboardRow {
                Key { key: Qt.Key_7; text: "7" }
                Key { key: Qt.Key_8; text: "8" }
                Key { key: Qt.Key_9; text: "9" }
            }
            KeyboardRow {
                Key { key: Qt.Key_4; text: "4" }
                Key { key: Qt.Key_5; text: "5" }
                Key { key: Qt.Key_6; text: "6" }
            }
            KeyboardRow {
                Key { key: Qt.Key_1; text: "1" }
                Key { key: Qt.Key_2; text: "2" }
                Key { key: Qt.Key_3; text: "3" }
            }
            KeyboardRow {
                Key { key: Qt.Key_0; text: "0" }
                Key { key: Qt.Key_Period; text: "." }
                Key { key: Qt.Key_Comma;  text: "," }
            }
        }

        FillerKey { weight: 40 }

        // ---- symbols ----------------------------------------------------
        KeyboardColumn {
            Layout.preferredWidth: 400

            KeyboardRow {
                Key { key: Qt.Key_At;         text: "@" }
                Key { key: Qt.Key_NumberSign; text: "#" }
                Key { key: Qt.Key_Dollar;     text: "$" }
                Key { key: Qt.Key_Percent;    text: "%" }
            }
            KeyboardRow {
                Key { key: Qt.Key_Plus;       text: "+" }
                Key { key: Qt.Key_Equal;      text: "=" }
                Key { key: Qt.Key_Asterisk;   text: "*" }
                Key { key: Qt.Key_Ampersand;  text: "&" }
            }
            KeyboardRow {
                Key { key: Qt.Key_Colon;      text: ":" }
                Key { key: Qt.Key_Semicolon;  text: ";" }
                Key { key: Qt.Key_ParenLeft;  text: "(" }
                Key { key: Qt.Key_ParenRight; text: ")" }
            }
            KeyboardRow {
                Key { key: Qt.Key_Slash;     text: "/"; alternativeKeys: "/?" }
                Key { key: Qt.Key_Backslash; text: "\\"; alternativeKeys: "\\|" }
                Key { key: Qt.Key_Question;  text: "?" }
                Key { key: Qt.Key_Exclam;    text: "!" }
            }
        }
    }
}

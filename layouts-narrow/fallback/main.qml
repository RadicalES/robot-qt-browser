// Narrow keyboard layout — four keys per row, nine rows.
//
// For short landscape panels, where the keyboard docks down the side of the
// page instead of under it. A T431 is 1024x600: a keyboard under the page can
// have at most a third of the height, and ten keys across the width that
// leaves gives 34px keys, which a finger cannot hit however the text is
// scaled. Docked at the side it has the full height and a quarter of the
// width — about 256x540 — and four keys across that is a comfortable 64px.
//
// The letters run in QWERTY order, reading left to right and top to bottom, so
// a hunt-and-peck operator finds them where they expect even though the rows
// wrap in an unfamiliar place.
//
// The standard ten-across layout is in layouts/ and is what a portrait panel
// like the T440 uses — 720px wide, it needs none of this.
//
// (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.VirtualKeyboard 2.1

KeyboardLayout {
    inputMode: InputEngine.InputMode.Latin
    keyWeight: 160

    KeyboardRow {
        Key { key: Qt.Key_Q; text: "q" }
        Key { key: Qt.Key_W; text: "w" }
        Key { key: Qt.Key_E; text: "e" }
        Key { key: Qt.Key_R; text: "r" }
    }

    KeyboardRow {
        Key { key: Qt.Key_T; text: "t" }
        Key { key: Qt.Key_Y; text: "y" }
        Key { key: Qt.Key_U; text: "u" }
        Key { key: Qt.Key_I; text: "i" }
    }

    KeyboardRow {
        Key { key: Qt.Key_O; text: "o" }
        Key { key: Qt.Key_P; text: "p" }
        Key { key: Qt.Key_A; text: "a" }
        Key { key: Qt.Key_S; text: "s" }
    }

    KeyboardRow {
        Key { key: Qt.Key_D; text: "d" }
        Key { key: Qt.Key_F; text: "f" }
        Key { key: Qt.Key_G; text: "g" }
        Key { key: Qt.Key_H; text: "h" }
    }

    KeyboardRow {
        Key { key: Qt.Key_J; text: "j" }
        Key { key: Qt.Key_K; text: "k" }
        Key { key: Qt.Key_L; text: "l" }
        Key { key: Qt.Key_Z; text: "z" }
    }

    KeyboardRow {
        Key { key: Qt.Key_X; text: "x" }
        Key { key: Qt.Key_C; text: "c" }
        Key { key: Qt.Key_V; text: "v" }
        Key { key: Qt.Key_B; text: "b" }
    }

    KeyboardRow {
        Key { key: Qt.Key_N; text: "n" }
        Key { key: Qt.Key_M; text: "m" }
        // The punctuation a terminal actually needs: a URL, a WiFi passphrase,
        // a station name. The rest is on the symbol page.
        Key { key: Qt.Key_Period; text: "."; alternativeKeys: ".,-_/@" }
        Key { key: Qt.Key_At; text: "@"; alternativeKeys: "@:;!?" }
    }

    KeyboardRow {
        ShiftKey {}
        SymbolModeKey {}
        BackspaceKey {}
        EnterKey {}
    }

    KeyboardRow {
        SpaceKey {}
    }
}

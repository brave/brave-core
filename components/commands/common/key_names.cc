// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/common/key_names.h"

#include "build/build_config.h"

namespace commands {

std::string GetKeyName(ui::KeyboardCode code) {
  switch (code) {
#if !BUILDFLAG(IS_WIN)
    case ui::VKEY_BACKTAB:
      return "Backtab";
    case ui::VKEY_NEW:
      return "New";
    case ui::VKEY_CLOSE:
      return "Close";
    case ui::VKEY_MEDIA_PLAY:
      return "Play";
    case ui::VKEY_MEDIA_PAUSE:
      return "Pause";
#endif
    case ui::VKEY_F1:
      return "F1";
    case ui::VKEY_F2:
      return "F2";
    case ui::VKEY_F3:
      return "F3";
    case ui::VKEY_F4:
      return "F4";
    case ui::VKEY_F5:
      return "F5";
    case ui::VKEY_F6:
      return "F6";
    case ui::VKEY_F7:
      return "F7";
    case ui::VKEY_F8:
      return "F8";
    case ui::VKEY_F9:
      return "F9";
    case ui::VKEY_F10:
      return "F10";
    case ui::VKEY_F11:
      return "F11";
    case ui::VKEY_F12:
      return "F12";
    case ui::VKEY_F13:
      return "F13";
    case ui::VKEY_F14:
      return "F14";
    case ui::VKEY_F15:
      return "F15";
    case ui::VKEY_F16:
      return "F16";
    case ui::VKEY_F17:
      return "F17";
    case ui::VKEY_F18:
      return "F18";
    case ui::VKEY_F19:
      return "F19";
    case ui::VKEY_F20:
      return "F20";
    case ui::VKEY_F21:
      return "F21";
    case ui::VKEY_F22:
      return "F22";
    case ui::VKEY_F23:
      return "F23";
    case ui::VKEY_F24:
      return "F24";
    case ui::VKEY_ESCAPE:
      return "Esc";
    case ui::VKEY_BROWSER_SEARCH:
      return "Search";
    case ui::VKEY_LMENU:
    case ui::VKEY_RMENU:
    case ui::VKEY_MENU:
      return "Alt";
    case ui::VKEY_BROWSER_FORWARD:
      return "Forward";
    case ui::VKEY_BROWSER_BACK:
      return "Back";
    case ui::VKEY_BROWSER_REFRESH:
      return "Refresh";
    case ui::VKEY_BROWSER_HOME:
      return "Home";
    case ui::VKEY_BROWSER_STOP:
      return "Stop";
    case ui::VKEY_BROWSER_FAVORITES:
      return "Favorites";
    case ui::VKEY_BACK:
      return "Back";
    case ui::VKEY_DELETE:
      return "Delete";
    case ui::VKEY_MEDIA_PLAY_PAUSE:
      return "Play Pause";
    case ui::VKEY_VOLUME_MUTE:
      return "Mute";
    case ui::VKEY_TAB:
      return "Tab";
    case ui::VKEY_NEXT:
      return "PgDn";
    case ui::VKEY_PRIOR:
      return "PgUp";
    case ui::VKEY_RETURN:
      return "Enter";
    case ui::VKEY_1:
    case ui::VKEY_NUMPAD1:
      return "1";
    case ui::VKEY_2:
    case ui::VKEY_NUMPAD2:
      return "2";
    case ui::VKEY_3:
    case ui::VKEY_NUMPAD3:
      return "3";
    case ui::VKEY_4:
    case ui::VKEY_NUMPAD4:
      return "4";
    case ui::VKEY_5:
    case ui::VKEY_NUMPAD5:
      return "5";
    case ui::VKEY_6:
    case ui::VKEY_NUMPAD6:
      return "6";
    case ui::VKEY_7:
    case ui::VKEY_NUMPAD7:
      return "7";
    case ui::VKEY_8:
    case ui::VKEY_NUMPAD8:
      return "8";
    case ui::VKEY_9:
    case ui::VKEY_NUMPAD9:
      return "9";
    case ui::VKEY_0:
    case ui::VKEY_NUMPAD0:
      return "0";
    case ui::VKEY_SUBTRACT:
    case ui::VKEY_OEM_MINUS:
      return "-";
    case ui::VKEY_ADD:
    case ui::VKEY_OEM_PLUS:
      return "+";
    case ui::VKEY_SPACE:
      return "Space";
    case ui::VKEY_LEFT:
      return "Left";
    case ui::VKEY_RIGHT:
      return "Right";
    case ui::VKEY_UP:
      return "Up";
    case ui::VKEY_DOWN:
      return "Down";
    case ui::VKEY_HOME:
      return "Home";
    case ui::VKEY_END:
      return "End";
    case ui::VKEY_CAPITAL:
      return "Caps";
    case ui::VKEY_A:
      return "A";
    case ui::VKEY_B:
      return "B";
    case ui::VKEY_C:
      return "C";
    case ui::VKEY_D:
      return "D";
    case ui::VKEY_E:
      return "E";
    case ui::VKEY_F:
      return "F";
    case ui::VKEY_G:
      return "G";
    case ui::VKEY_H:
      return "H";
    case ui::VKEY_I:
      return "I";
    case ui::VKEY_J:
      return "J";
    case ui::VKEY_K:
      return "K";
    case ui::VKEY_L:
      return "L";
    case ui::VKEY_M:
      return "M";
    case ui::VKEY_N:
      return "N";
    case ui::VKEY_O:
      return "O";
    case ui::VKEY_P:
      return "P";
    case ui::VKEY_Q:
      return "Q";
    case ui::VKEY_R:
      return "R";
    case ui::VKEY_S:
      return "S";
    case ui::VKEY_T:
      return "T";
    case ui::VKEY_U:
      return "U";
    case ui::VKEY_V:
      return "V";
    case ui::VKEY_W:
      return "W";
    case ui::VKEY_X:
      return "X";
    case ui::VKEY_Y:
      return "Y";
    case ui::VKEY_Z:
      return "Z";
    case ui::VKEY_CANCEL:
      return "Cancel";
    case ui::VKEY_CLEAR:
      return "Clear";
    case ui::VKEY_SHIFT:
      return "Shift";
    case ui::VKEY_CONTROL:
      return "Ctrk";
    case ui::VKEY_PAUSE:
      return "Pause";
    case ui::VKEY_KANA:
      return "Kana";
    case ui::VKEY_PASTE:
      return "Paste";
    case ui::VKEY_JUNJA:
      return "Junja";
    case ui::VKEY_FINAL:
      return "Final";
    case ui::VKEY_HANJA:
      return "Hanja";
    case ui::VKEY_CONVERT:
      return "Convert";
    case ui::VKEY_NONCONVERT:
      return "Non Convert";
    case ui::VKEY_ACCEPT:
      return "";
    case ui::VKEY_MODECHANGE:
      return "Mode";
    case ui::VKEY_SELECT:
      return "Select";
    case ui::VKEY_PRINT:
      return "Print";
    case ui::VKEY_EXECUTE:
      return "Execute";
    case ui::VKEY_SNAPSHOT:
      return "PrtScn";
    case ui::VKEY_INSERT:
      return "Ins";
    case ui::VKEY_HELP:
      return "Help";
    case ui::VKEY_RWIN:
    case ui::VKEY_COMMAND:
      return "Cmd";
    default:
      return "Unknown";
  }
}

std::vector<std::string> GetModifierName(ui::KeyEventFlags flags) {
  std::vector<std::string> result;

  if (flags & ui::EF_COMMAND_DOWN) {
    result.push_back("Cmd");
  }

  if (flags & ui::EF_CONTROL_DOWN) {
    result.push_back("Ctrl");
  }

  if (flags & ui::EF_ALT_DOWN) {
    result.push_back("Alt");
  }

  if (flags & ui::EF_SHIFT_DOWN) {
    result.push_back("Shift");
  }

  if (flags & ui::EF_FUNCTION_DOWN) {
    result.push_back("Fn");
  }

  return result;
}

}  // namespace commands

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/build_devtools_key_event_params.h"

#include <string>
#include <string_view>
#include <unordered_map>

#include "ui/events/event_constants.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace ai_chat {

namespace {
// A mapping from xdotool keys → Chromium ui::KeyboardCode.
static const std::unordered_map<std::string, ui::KeyboardCode>
    kXdotoolToKeyboardCodeMap = {
        {"Page_Down", ui::VKEY_NEXT}, {"Page_Up", ui::VKEY_PRIOR},
        {"Home", ui::VKEY_HOME},      {"End", ui::VKEY_END},
        {"Return", ui::VKEY_RETURN},  {"Enter", ui::VKEY_RETURN},
        {"Escape", ui::VKEY_ESCAPE},
        // TODO(petemill): Add more mappings as needed
};

// Returns a bitmask of ui::EF_* flags (ctrl, alt, shift, meta) based on the prefix tokens.
int ParseModifiers(const std::vector<std::string>& tokens) {
  int modifiers = 0;
  for (const auto& token : tokens) {
    std::string lower = base::ToLowerASCII(token);
    if (lower == "ctrl") {
      modifiers |= ui::EF_CONTROL_DOWN;
    } else if (lower == "shift") {
      modifiers |= ui::EF_SHIFT_DOWN;
    } else if (lower == "alt") {
      modifiers |= ui::EF_ALT_DOWN;
    } else if (lower == "meta" || lower == "super") {
      modifiers |= ui::EF_COMMAND_DOWN;
    }
  }
  return modifiers;
}

}

DevToolsKeyEventParams BuildDevToolsKeyEventParams(std::string_view xdotool_key) {
  DevToolsKeyEventParams result = {0, 0, "", 0};

  // Split on '+' to detect modifiers vs. final key.
  // e.g., "ctrl+shift+a" → ["ctrl", "shift", "a"]
  std::vector<std::string> tokens = base::SplitString(
      xdotool_key, "+", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::string last_token = tokens.back();
  tokens.pop_back();

  // Parse the modifiers (ctrl, shift, alt, meta/super).
  result.modifiers = ParseModifiers(tokens);

  ui::KeyboardCode keyboard_code = ui::VKEY_UNKNOWN;

  if (last_token.size() == 1) {
    // Single character. For example 'a' → VKEY_A
    // TODO(petemill): Try to use kPrintableCodeMap to find dom code
    // ...and on mac only we can use:
    // ui::KeyboardCodeFromCharCode(last_token[0]);
    char c = last_token[0];
    if (c >= 'a' && c <= 'z') {
      keyboard_code = static_cast<ui::KeyboardCode>(ui::VKEY_A + (c - 'a'));
    }
  } else {
    // Map the xdotool key to a Chromium ui::KeyboardCode.
    auto it = kXdotoolToKeyboardCodeMap.find(last_token);
    if (it != kXdotoolToKeyboardCodeMap.end()) {
      keyboard_code = it->second;
    }
  }

  // Convert the ui::KeyboardCode into a DomCode via US layout
  // (this is a best-effort approach, but usually correct for special keys).
  ui::DomCode dom_code =
      (keyboard_code != ui::VKEY_UNKNOWN)
          ? ui::UsLayoutKeyboardCodeToDomCode(keyboard_code)
          // Last effort
          : ui::UsLayoutDomKeyToDomCode(ui::KeycodeConverter::KeyStringToDomKey(last_token));

  if (dom_code == ui::DomCode::NONE) {
    dom_code = ui::KeycodeConverter::CodeStringToDomCode(last_token);
  }

  // Convert that DomCode into a DOM code string (e.g. "PageDown", "PageUp", etc.).
  std::string dom_code_string =
      (dom_code != ui::DomCode::NONE)
          ? ui::KeycodeConverter::DomCodeToCodeString(dom_code)
          : last_token;

  result.dom_code_string = dom_code_string;

  // Determine the correct native virtual key code (platform-specific).
  // On Windows, we can just cast ui::KeyboardCode to int. On Linux, we
  // typically use DomCode → native keycode. For simplicity, we show a cross-platform pattern:
  // On Windows, ui::KeyboardCode maps to the actual Win32 virtual-key code.
  result.windows_native_virtual_key_code = static_cast<int>(keyboard_code);
  result.native_virtual_key_code = ui::KeycodeConverter::DomCodeToNativeKeycode(dom_code);

  return result;
}

}  // namespace ai_chat

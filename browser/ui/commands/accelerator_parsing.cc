// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/accelerator_parsing.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"

namespace commands {

namespace {
constexpr char kApplicationClose[] = "AppClose";
constexpr char kApplicationNew[] = "AppNew";
std::string KeyboardCodeToDomCodeString(ui::KeyboardCode code) {
  if (code == ui::VKEY_CLOSE) {
    return kApplicationClose;
  }

  if (code == ui::VKEY_NEW) {
    return kApplicationNew;
  }

  auto domcode = ui::UsLayoutKeyboardCodeToDomCode(code);
  return ui::KeycodeConverter::DomCodeToCodeString(domcode);
}

ui::KeyboardCode DomCodeStringToKeyboardCode(const std::string& key) {
  if (key == kApplicationClose) {
    return ui::VKEY_CLOSE;
  }

  if (key == kApplicationNew) {
    return ui::VKEY_NEW;
  }

  auto domcode = ui::KeycodeConverter::CodeStringToDomCode(key);
  return ui::DomCodeToUsLayoutKeyboardCode(domcode);
}

std::vector<std::string> GetModifierNames(ui::KeyEventFlags flags) {
  std::vector<std::string> result;

  if (flags & ui::EF_COMMAND_DOWN) {
    result.push_back("Meta");
  }

  if (flags & ui::EF_CONTROL_DOWN) {
    result.push_back("Control");
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

ui::KeyEventFlags GetModifierFromKeys(
    const std::vector<std::string>& modifiers) {
  ui::KeyEventFlags result = ui::EF_NONE;
  if (base::Contains(modifiers, "Control")) {
    result = result | ui::EF_CONTROL_DOWN;
  }
  if (base::Contains(modifiers, "Meta")) {
    result = result | ui::EF_COMMAND_DOWN;
  }
  if (base::Contains(modifiers, "Alt")) {
    result = result | ui::EF_ALT_DOWN;
  }
  if (base::Contains(modifiers, "Shift")) {
    result = result | ui::EF_SHIFT_DOWN;
  }
  if (base::Contains(modifiers, "Fn")) {
    result = result | ui::EF_FUNCTION_DOWN;
  }
  return result;
}
}  // namespace

std::string ToKeysString(const ui::Accelerator& accelerator) {
  std::vector<std::string> parts = GetModifierNames(accelerator.modifiers());
  auto domcode = ui::UsLayoutKeyboardCodeToDomCode(accelerator.key_code());
  ui::DomKey domkey;
  ui::KeyboardCode out_code;
  if (!ui::DomCodeToUsLayoutDomKey(domcode, 0, &domkey, &out_code)) {
    parts.push_back("Unknown Key: " +
                    base::NumberToString(accelerator.key_code()));
  } else {
    parts.push_back(ui::KeycodeConverter::DomKeyToKeyString(domkey));
  }
  return base::JoinString(parts, "+");
}

std::string ToCodesString(const ui::Accelerator& accelerator) {
  auto parts = GetModifierNames(accelerator.modifiers());
  parts.push_back(KeyboardCodeToDomCodeString(accelerator.key_code()));
  return base::JoinString(parts, "+");
}

ui::Accelerator FromCodesString(const std::string& value) {
  std::vector<std::string> parts = base::SplitString(
      value, "+", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  auto keyname = parts[parts.size() - 1];
  auto keycode = DomCodeStringToKeyboardCode(keyname);

  std::vector<std::string> modifiers(parts.begin(), parts.end() - 1);
  return ui::Accelerator(keycode, GetModifierFromKeys(modifiers));
}

}  // namespace commands

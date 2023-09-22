// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/common/accelerator_parsing.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace commands {

namespace {

#if !BUILDFLAG(IS_WIN)
constexpr char kApplicationClose[] = "AppClose";
constexpr char kApplicationNew[] = "AppNew";
#endif

constexpr char kMenu[] = "Alt";
constexpr char kRMenu[] = "AltGr";

struct ModifierName {
  const ui::KeyEventFlags modifier;
  const std::string_view name;
};

const std::vector<ModifierName>& GetAllModifierNames() {
  static const base::NoDestructor<std::vector<ModifierName>> kModifierNames({
#if BUILDFLAG(IS_MAC)
    {.modifier = ui::EF_COMMAND_DOWN, .name = "Command"},
#else
    {.modifier = ui::EF_COMMAND_DOWN, .name = "Meta"},
#endif
        {.modifier = ui::EF_CONTROL_DOWN, .name = "Control"},
        {.modifier = ui::EF_ALT_DOWN, .name = "Alt"},
        {.modifier = ui::EF_ALTGR_DOWN, .name = "AltGr"},
        {.modifier = ui::EF_SHIFT_DOWN, .name = "Shift"},
        {.modifier = ui::EF_FUNCTION_DOWN, .name = "Fn"},
  });

  return *kModifierNames;
}

std::string KeyboardCodeToDomCodeString(ui::KeyboardCode code) {
#if !BUILDFLAG(IS_WIN)
  if (code == ui::VKEY_CLOSE) {
    return kApplicationClose;
  }

  if (code == ui::VKEY_NEW) {
    return kApplicationNew;
  }
#endif

  if (code == ui::VKEY_LMENU || code == ui::VKEY_MENU) {
    return kMenu;
  }

  if (code == ui::VKEY_RMENU) {
    return kRMenu;
  }

  auto domcode = ui::UsLayoutKeyboardCodeToDomCode(code);
  return ui::KeycodeConverter::DomCodeToCodeString(domcode);
}

ui::KeyboardCode DomCodeStringToKeyboardCode(const std::string& key) {
#if !BUILDFLAG(IS_WIN)
  if (key == kApplicationClose) {
    return ui::VKEY_CLOSE;
  }

  if (key == kApplicationNew) {
    return ui::VKEY_NEW;
  }
#endif

  if (key == kMenu) {
    return ui::VKEY_MENU;
  }

  if (key == kRMenu) {
    return ui::VKEY_RMENU;
  }

  auto domcode = ui::KeycodeConverter::CodeStringToDomCode(key);
  return ui::DomCodeToUsLayoutKeyboardCode(domcode);
}

std::vector<std::string> GetModifierNames(ui::KeyEventFlags flags) {
  std::vector<std::string> result;
  for (const auto& [modifier, name] : GetAllModifierNames()) {
    if (flags & modifier) {
      result.emplace_back(name);
    }
  }

  return result;
}

ui::KeyEventFlags GetModifierFromKeys(
    const std::vector<std::string>& modifiers) {
  ui::KeyEventFlags result = ui::EF_NONE;
  for (const auto& [modifier, name] : GetAllModifierNames()) {
    if (base::Contains(modifiers, name)) {
      result |= modifier;
    }
  }
  return result;
}

std::string KeyCodeToString(ui::KeyboardCode key_code) {
  switch (key_code) {
    case ui::VKEY_LMENU:
    case ui::VKEY_MENU:
      return kMenu;
    case ui::VKEY_RMENU:
      return kRMenu;
    case ui::VKEY_NUMPAD0:
      return "Num0";
    case ui::VKEY_NUMPAD1:
      return "Num1";
    case ui::VKEY_NUMPAD2:
      return "Num2";
    case ui::VKEY_NUMPAD3:
      return "Num3";
    case ui::VKEY_NUMPAD4:
      return "Num4";
    case ui::VKEY_NUMPAD5:
      return "Num5";
    case ui::VKEY_NUMPAD6:
      return "Num6";
    case ui::VKEY_NUMPAD7:
      return "Num7";
    case ui::VKEY_NUMPAD8:
      return "Num8";
    case ui::VKEY_NUMPAD9:
      return "Num9";
    case ui::VKEY_ADD:
      return "NumAdd";
    case ui::VKEY_SUBTRACT:
      return "NumSubtract";
    case ui::VKEY_MULTIPLY:
      return "NumMultiply";
    case ui::VKEY_DIVIDE:
      return "NumDivide";
    case ui::VKEY_DECIMAL:
      return "NumDecimal";
    default:
      auto domcode = ui::UsLayoutKeyboardCodeToDomCode(key_code);
      ui::DomKey domkey;
      ui::KeyboardCode out_code;
      if (!ui::DomCodeToUsLayoutDomKey(domcode, 0, &domkey, &out_code)) {
        return "Unknown Key: " + base::NumberToString(key_code);
      }
      return ui::KeycodeConverter::DomKeyToKeyString(domkey);
  }
}

}  // namespace

std::string CodeStringToKeyString(const std::string& code_string) {
  auto code = DomCodeStringToKeyboardCode(code_string);
  return KeyCodeToString(code);
}

std::string ToKeysString(const ui::Accelerator& accelerator) {
  std::vector<std::string> parts = GetModifierNames(accelerator.modifiers());
  parts.push_back(KeyCodeToString(accelerator.key_code()));
  return base::JoinString(parts, "+");
}

std::string ToCodesString(const ui::Accelerator& accelerator) {
  auto parts = GetModifierNames(accelerator.modifiers());
  parts.push_back(KeyboardCodeToDomCodeString(accelerator.key_code()));
  return base::JoinString(parts, "+");
}

ui::Accelerator FromCodesString(const std::string& value) {
  DCHECK(!value.empty());
  std::vector<std::string> parts = base::SplitString(
      value, "+", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  // Not sure why, but some clients are encountering empty accelerators. If we
  // encounter one in the wild, just return an empty accelerator instead of
  // crashing:
  // https://github.com/brave/brave-browser/issues/31419
  if (parts.empty()) {
    return ui::Accelerator();
  }

  auto keyname = parts[parts.size() - 1];
  auto keycode = DomCodeStringToKeyboardCode(keyname);

  std::vector<std::string> modifiers(parts.begin(), parts.end() - 1);
  return ui::Accelerator(keycode, GetModifierFromKeys(modifiers));
}

}  // namespace commands

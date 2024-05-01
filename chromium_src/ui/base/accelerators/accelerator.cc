// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/accelerators/accelerator.h"

#include <string>

#include "components/grit/brave_components_strings.h"
#include "ui/strings/grit/ui_strings.h"

namespace ui {
namespace {
std::u16string ApplyModifierToAcceleratorString(const std::u16string&, int);

// We make significant changes to the `ApplyLongFormModifiers` method, and
// instead of adding multiple patches it's easier to implement our own version.
std::u16string BraveApplyLongFormModifiers(const std::u16string& shortcut,
                                           bool shift,
                                           bool ctrl,
                                           bool alt,
                                           bool cmd,
                                           bool fn) {
  std::u16string result = shortcut;
  if (ctrl) {
    result = ApplyModifierToAcceleratorString(result, IDS_APP_CTRL_KEY);
  }

  if (shift) {
    result = ApplyModifierToAcceleratorString(result, IDS_APP_SHIFT_KEY);
  }

  if (alt) {
    result = ApplyModifierToAcceleratorString(result, IDS_APP_ALT_KEY);
  }

  if (cmd) {
#if BUILDFLAG(IS_MAC)
    result = ApplyModifierToAcceleratorString(result, IDS_APP_COMMAND_KEY);
#elif BUILDFLAG(IS_WIN)
    result = ApplyModifierToAcceleratorString(result, IDS_APP_WINDOWS_KEY);
#else
    result = ApplyModifierToAcceleratorString(result, IDS_APP_META_KEY);
#endif
  }

  return result;
}
}  // namespace
}  // namespace ui

// Upstream doesn't support accelerators with Control+Alt - we do, but only for
// user defined shortcuts.
#define BRAVE_UI_BASE_ACCELERATOR_APPLY_LONG_FORM_MODIFIERS_                \
  return BraveApplyLongFormModifiers(shortcut, IsShiftDown(), IsCtrlDown(), \
                                     IsAltDown(), IsCmdDown(),              \
                                     IsFunctionDown());

#include "src/ui/base/accelerators/accelerator.cc"

#undef BRAVE_UI_BASE_ACCELERATOR_APPLY_LONG_FORM_MODIFIERS_

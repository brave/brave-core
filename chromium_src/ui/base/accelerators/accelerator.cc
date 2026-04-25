// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/accelerators/accelerator.h"

#include <string>

#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

namespace ui {
namespace {

// We make significant changes to the `ApplyLongFormModifiers` method, and
// instead of adding multiple patches it's easier to implement our own version.
std::vector<std::u16string> BraveGetLongFormModifiers(bool shift,
                                                      bool ctrl,
                                                      bool alt,
                                                      bool cmd,
                                                      bool fn) {
  std::vector<std::u16string> modifiers;
  if (cmd) {
#if BUILDFLAG(IS_MAC)
    modifiers.push_back(l10n_util::GetStringUTF16(IDS_APP_COMMAND_KEY));
#elif BUILDFLAG(IS_WIN)
    modifiers.push_back(l10n_util::GetStringUTF16(IDS_APP_WINDOWS_KEY));
#else
    modifiers.push_back(l10n_util::GetStringUTF16(IDS_APP_META_KEY));
#endif
  }

  if (ctrl) {
    modifiers.push_back(l10n_util::GetStringUTF16(IDS_APP_CTRL_KEY));
  }

  if (shift) {
    modifiers.push_back(l10n_util::GetStringUTF16(IDS_APP_SHIFT_KEY));
  }

  if (alt) {
    modifiers.push_back(l10n_util::GetStringUTF16(IDS_APP_ALT_KEY));
  }

  return modifiers;
}
}  // namespace
}  // namespace ui

// Upstream doesn't support accelerators with Control+Alt - we do, but only for
// user defined shortcuts.
#define BRAVE_UI_BASE_ACCELERATOR_GET_LONG_FORM_MODIFIERS                    \
  return BraveGetLongFormModifiers(IsShiftDown(), IsCtrlDown(), IsAltDown(), \
                                   IsCmdDown(), IsFunctionDown());

#include <ui/base/accelerators/accelerator.cc>

#undef BRAVE_UI_BASE_ACCELERATOR_GET_LONG_FORM_MODIFIERS

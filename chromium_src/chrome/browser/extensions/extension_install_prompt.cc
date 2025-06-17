/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_install_prompt.h"

#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

#define GetDialogTitle GetDialogTitle_ChromiumImpl
#include <chrome/browser/extensions/extension_install_prompt.cc>
#undef GetDialogTitle

std::u16string ExtensionInstallPrompt::Prompt::GetDialogTitle() const {
  if (type_ == ExtensionInstallPrompt::INSTALL_PROMPT) {
    return l10n_util::GetStringUTF16(
        IDS_UNVETTED_EXTENSION_INSTALL_PROMPT_TITLE);
  }
  return GetDialogTitle_ChromiumImpl();
}

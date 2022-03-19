/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_install_prompt.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/extension_whitelist_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "extensions/common/extension.h"
#include "ui/base/l10n/l10n_util.h"

#define GetDialogTitle GetDialogTitle_ChromiumImpl
#include "src/chrome/browser/extensions/extension_install_prompt.cc"
#undef GetDialogTitle

std::u16string ExtensionInstallPrompt::Prompt::GetDialogTitle() const {
  if (!g_brave_browser_process->extension_whitelist_service()->IsVetted(
          extension()->id())) {
    if (type_ == ExtensionInstallPrompt::INSTALL_PROMPT ||
        type_ == ExtensionInstallPrompt::WEBSTORE_WIDGET_PROMPT) {
      return l10n_util::GetStringUTF16(
          IDS_UNVETTED_EXTENSION_INSTALL_PROMPT_TITLE);
    }
  }
  return GetDialogTitle_ChromiumImpl();
}

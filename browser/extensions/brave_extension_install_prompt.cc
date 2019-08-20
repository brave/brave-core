/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_install_prompt.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_component_updater/browser/extension_whitelist_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

base::string16 BravePrompt::GetDialogTitle() const {
  if (!g_brave_browser_process->extension_whitelist_service()->IsVetted(
      extension())) {
    if (type_ == ExtensionInstallPrompt::INSTALL_PROMPT ||
        type_ == ExtensionInstallPrompt::WEBSTORE_WIDGET_PROMPT) {
      return l10n_util::GetStringUTF16(
          IDS_UNVETTED_EXTENSION_INSTALL_PROMPT_TITLE);
    }
  }
  return ExtensionInstallPrompt::Prompt::GetDialogTitle();
}

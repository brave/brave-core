/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_install_prompt.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/extensions/brave_extension_provider.h"
#include "brave/grit/brave_generated_resources.h"
#include "extensions/common/extension_id.h"
#include "ui/base/l10n/l10n_util.h"

BravePrompt::BravePrompt(ExtensionInstallPrompt::PromptType type) :
    ExtensionInstallPrompt::Prompt(type) {
}

BravePrompt::~BravePrompt() {
}

base::string16 BravePrompt::GetDialogTitle() const {
  if (!extensions::BraveExtensionProvider::IsVetted(extension())) {
    if (type_ == ExtensionInstallPrompt::INSTALL_PROMPT ||
        type_ == ExtensionInstallPrompt::INLINE_INSTALL_PROMPT) {
      return l10n_util::GetStringFUTF16(IDS_UNVETTED_EXTENSION_INSTALL_PROMPT_TITLE,
          base::UTF8ToUTF16(extension_->name()));
    }
  }
  return ExtensionInstallPrompt::Prompt::GetDialogTitle();
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_INSTALL_PROMPT_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_INSTALL_PROMPT_H_

#include "chrome/browser/extensions/extension_install_prompt.h"

class BravePrompt : public ExtensionInstallPrompt::Prompt {
 public:
  explicit BravePrompt(ExtensionInstallPrompt::PromptType type);
  ~BravePrompt() override;

  base::string16 GetDialogTitle() const override;

  DISALLOW_COPY_AND_ASSIGN(BravePrompt);
};

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_INSTALL_PROMPT_H_

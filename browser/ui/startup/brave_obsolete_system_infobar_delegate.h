/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_STARTUP_BRAVE_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_UI_STARTUP_BRAVE_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_

#include <string>

#include "chrome/browser/ui/startup/obsolete_system_infobar_delegate.h"

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

// Subclassed for showing "Don't show again" button.
// W/o this button, user will see this infobar whenever launched.
class BraveObsoleteSystemInfoBarDelegate
    : public ObsoleteSystemInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager);

  BraveObsoleteSystemInfoBarDelegate(
      const BraveObsoleteSystemInfoBarDelegate&) = delete;
  BraveObsoleteSystemInfoBarDelegate& operator=(
      const BraveObsoleteSystemInfoBarDelegate&) = delete;

 private:
  BraveObsoleteSystemInfoBarDelegate();
  ~BraveObsoleteSystemInfoBarDelegate() override;

  // ObsoleteSystemInfoBarDelegate overrides:
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
};

#endif  // BRAVE_BROWSER_UI_STARTUP_BRAVE_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_

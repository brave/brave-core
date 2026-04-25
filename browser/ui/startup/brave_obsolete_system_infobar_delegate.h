/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_STARTUP_BRAVE_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_UI_STARTUP_BRAVE_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

class BraveObsoleteSystemInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager);

  BraveObsoleteSystemInfoBarDelegate(
      const BraveObsoleteSystemInfoBarDelegate&) = delete;
  BraveObsoleteSystemInfoBarDelegate& operator=(
      const BraveObsoleteSystemInfoBarDelegate&) = delete;

 private:
  BraveObsoleteSystemInfoBarDelegate();
  ~BraveObsoleteSystemInfoBarDelegate() override;

  // BraveConfirmInfoBarDelegate overrides:
  bool HasCheckbox() const override;
  std::u16string GetCheckboxText() const override;
  void SetCheckboxChecked(bool checked) override;
  bool InterceptClosing() override;
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::vector<int> GetButtonsOrder() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;

  void OnConfirmDialogClosing(bool suppress);

  bool launch_confirmation_dialog_ = false;

  base::WeakPtrFactory<BraveObsoleteSystemInfoBarDelegate> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_STARTUP_BRAVE_OBSOLETE_SYSTEM_INFOBAR_DELEGATE_H_

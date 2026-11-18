/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_SHIELDS_RELOAD_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_SHIELDS_RELOAD_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

class BraveShieldsReloadInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  BraveShieldsReloadInfoBarDelegate(const BraveShieldsReloadInfoBarDelegate&) =
      delete;
  BraveShieldsReloadInfoBarDelegate& operator=(
      const BraveShieldsReloadInfoBarDelegate&) = delete;
  ~BraveShieldsReloadInfoBarDelegate() override;

  // Creates a reload infobar and adds it to |infobar_manager| if one is not
  // already present.
  static void Create(infobars::ContentInfoBarManager* infobar_manager);

 private:
  BraveShieldsReloadInfoBarDelegate();

  // ConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_SHIELDS_RELOAD_INFOBAR_DELEGATE_H_

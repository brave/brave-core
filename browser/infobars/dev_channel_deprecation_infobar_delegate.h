/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_DEV_CHANNEL_DEPRECATION_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_DEV_CHANNEL_DEPRECATION_INFOBAR_DELEGATE_H_

#include <vector>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

class DevChannelDeprecationInfoBarDelegate
    : public BraveConfirmInfoBarDelegate {
 public:
  static void CreateIfNeeded(infobars::InfoBarManager* infobar_manager);

  DevChannelDeprecationInfoBarDelegate(
      const DevChannelDeprecationInfoBarDelegate&) = delete;
  DevChannelDeprecationInfoBarDelegate& operator=(
      const DevChannelDeprecationInfoBarDelegate&) = delete;

  ~DevChannelDeprecationInfoBarDelegate() override;

 private:
  DevChannelDeprecationInfoBarDelegate();

  // BraveConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::vector<int> GetButtonsOrder() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
};

#endif  // BRAVE_BROWSER_INFOBARS_DEV_CHANNEL_DEPRECATION_INFOBAR_DELEGATE_H_

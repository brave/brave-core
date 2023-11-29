/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/browser/infobars/brave_global_confirm_infobar_delegate.h"

class PrefService;

class BraveIPFSAlwaysStartInfoBarDelegate
    : public BraveGlobalConfirmInfobarDelegate {
 public:
  BraveIPFSAlwaysStartInfoBarDelegate(
      const BraveIPFSAlwaysStartInfoBarDelegate&) = delete;
  BraveIPFSAlwaysStartInfoBarDelegate& operator=(
      const BraveIPFSAlwaysStartInfoBarDelegate&) = delete;
  ~BraveIPFSAlwaysStartInfoBarDelegate() override;

 private:
  friend class BraveIPFSAlwaysStartInfoBarDelegateFactory;
  explicit BraveIPFSAlwaysStartInfoBarDelegate(PrefService* local_state);

  // BraveConfirmInfoBarDelegate
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  std::vector<int> GetButtonsOrder() const override;

  bool Accept() override;
  bool Cancel() override;
  void InfoBarDismissed() override;

  void SetLastShownTime();

  raw_ptr<PrefService> local_state_ = nullptr;
};

class BraveIPFSAlwaysStartInfoBarDelegateFactory
    : public BraveGlobalConfirmInfoBarDelegateFactory {
 public:
  explicit BraveIPFSAlwaysStartInfoBarDelegateFactory(PrefService* local_state);
  ~BraveIPFSAlwaysStartInfoBarDelegateFactory() override = default;

  std::unique_ptr<BraveGlobalConfirmInfobarDelegate> Create() override;
  infobars::InfoBarDelegate::InfoBarIdentifier GetInfoBarIdentifier()
      const override;

 private:
  raw_ptr<PrefService> local_state_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE_H_

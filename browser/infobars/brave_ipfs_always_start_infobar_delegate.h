/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

class PrefService;

namespace ipfs {
  class IpfsService;
}  // namespace ipfs

namespace infobars {
  class ContentInfoBarManager;
}

class BraveIPFSAlwaysStartInfoBarDelegate : public BraveConfirmInfoBarDelegate {
public:
  BraveIPFSAlwaysStartInfoBarDelegate(const BraveIPFSAlwaysStartInfoBarDelegate&) =
      delete;
  BraveIPFSAlwaysStartInfoBarDelegate& operator=(
      const BraveIPFSAlwaysStartInfoBarDelegate&) = delete;

private:
  friend class BraveIPFSAlwaysStartInfoBarDelegateFactory;
  explicit BraveIPFSAlwaysStartInfoBarDelegate(ipfs::IpfsService* ipfs_service, PrefService* local_state);

  ~BraveIPFSAlwaysStartInfoBarDelegate() override;

  // BraveConfirmInfoBarDelegate
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  std::vector<int> GetButtonsOrder() const override;

  bool Accept() override;
  bool Cancel() override;

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<ipfs::IpfsService> ipfs_service_ = nullptr;
};

class BraveIPFSAlwaysStartInfoBarDelegateFactory : public BraveConfirmInfoBarDelegateFactory {
public:
  explicit BraveIPFSAlwaysStartInfoBarDelegateFactory(ipfs::IpfsService* ipfs_service, PrefService* local_state);
  ~BraveIPFSAlwaysStartInfoBarDelegateFactory() override = default;

  void Create() override;
private:
  raw_ptr<PrefService> local_state_ = nullptr;
};

#endif // BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE_H_
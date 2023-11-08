/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_INFOBAR_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "url/gurl.h"

class PrefService;

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

class BraveIPFSInfoBarDelegateObserver {
 public:
  BraveIPFSInfoBarDelegateObserver();
  virtual void OnRedirectToIPFS(bool remember) = 0;
  virtual ~BraveIPFSInfoBarDelegateObserver();
};

class BraveIPFSInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  BraveIPFSInfoBarDelegate(const BraveIPFSInfoBarDelegate&) = delete;
  BraveIPFSInfoBarDelegate& operator=(const BraveIPFSInfoBarDelegate&) = delete;

  ~BraveIPFSInfoBarDelegate() override;

 private:
  friend class BraveIPFSInfoBarDelegateFactory;
  explicit BraveIPFSInfoBarDelegate(
      std::unique_ptr<BraveIPFSInfoBarDelegateObserver> observer,
      PrefService* local_state);

  // BraveConfirmInfoBarDelegate
  bool HasCheckbox() const override;
  std::u16string GetCheckboxText() const override;
  void SetCheckboxChecked(bool checked) override;
  bool InterceptClosing() override;

  // ConfirmInfoBarDelegate
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::vector<int> GetButtonsOrder() const override;
  bool IsProminent(int id) const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;

  bool Accept() override;
  bool Cancel() override;
  bool ExtraButtonPressed() override;

  std::unique_ptr<BraveIPFSInfoBarDelegateObserver> observer_;
  raw_ptr<PrefService> local_state_ = nullptr;
};

class BraveIPFSInfoBarDelegateFactory : public BraveConfirmInfoBarDelegateFactory {
public:
  BraveIPFSInfoBarDelegateFactory(infobars::ContentInfoBarManager* infobar_manager,
    std::unique_ptr<BraveIPFSInfoBarDelegateObserver> observer,
    PrefService* local_state);
  ~BraveIPFSInfoBarDelegateFactory() override = default;

  void Create() override;
private:
  raw_ptr<PrefService> local_state_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_INFOBAR_DELEGATE_H_

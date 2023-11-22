/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_FALLBACK_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_FALLBACK_INFOBAR_DELEGATE_H_

#include <memory>
#include <vector>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"

class PrefService;

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

class BraveIPFSFallbackInfoBarDelegateObserver {
 public:
  BraveIPFSFallbackInfoBarDelegateObserver();
  virtual void OnRedirectToOriginalAddress() = 0;
  virtual ~BraveIPFSFallbackInfoBarDelegateObserver();
};

class BraveIPFSFallbackInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  BraveIPFSFallbackInfoBarDelegate(const BraveIPFSFallbackInfoBarDelegate&) =
      delete;
  BraveIPFSFallbackInfoBarDelegate& operator=(
      const BraveIPFSFallbackInfoBarDelegate&) = delete;

  static void Create(
      infobars::ContentInfoBarManager* infobar_manager,
      std::unique_ptr<BraveIPFSFallbackInfoBarDelegateObserver> observer,
      PrefService* local_state);

 private:
  explicit BraveIPFSFallbackInfoBarDelegate(
      std::unique_ptr<BraveIPFSFallbackInfoBarDelegateObserver> observer,
      PrefService* local_state);
  ~BraveIPFSFallbackInfoBarDelegate() override;

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
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  std::vector<int> GetButtonsOrder() const override;

  bool Accept() override;
  bool Cancel() override;

  std::unique_ptr<BraveIPFSFallbackInfoBarDelegateObserver> observer_;
  raw_ptr<PrefService> local_state_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_IPFS_FALLBACK_INFOBAR_DELEGATE_H_

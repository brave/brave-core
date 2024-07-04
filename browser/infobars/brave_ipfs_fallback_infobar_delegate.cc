/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_ipfs_fallback_infobar_delegate.h"

#include <utility>

#include "brave/browser/infobars/brave_confirm_infobar_creator.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/infobars/core/infobar.h"

// BraveIPFSFallbackInfoBarDelegateObserver
BraveIPFSFallbackInfoBarDelegateObserver::
    BraveIPFSFallbackInfoBarDelegateObserver() = default;
BraveIPFSFallbackInfoBarDelegateObserver::
    ~BraveIPFSFallbackInfoBarDelegateObserver() = default;

// BraveIPFSFallbackInfoBarDelegate
// static
void BraveIPFSFallbackInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    std::unique_ptr<BraveIPFSFallbackInfoBarDelegateObserver> observer,
    PrefService* local_state) {
  infobar_manager->AddInfoBar(
      CreateBraveConfirmInfoBar(std::unique_ptr<BraveConfirmInfoBarDelegate>(
          new BraveIPFSFallbackInfoBarDelegate(std::move(observer),
                                               local_state))),
      true);
}

BraveIPFSFallbackInfoBarDelegate::BraveIPFSFallbackInfoBarDelegate(
    std::unique_ptr<BraveIPFSFallbackInfoBarDelegateObserver> observer,
    PrefService* local_state)
    : observer_(std::move(observer)), local_state_(local_state) {}

BraveIPFSFallbackInfoBarDelegate::~BraveIPFSFallbackInfoBarDelegate() = default;

// BraveConfirmInfoBarDelegate
bool BraveIPFSFallbackInfoBarDelegate::HasCheckbox() const {
  return false;
}

std::u16string BraveIPFSFallbackInfoBarDelegate::GetCheckboxText() const {
  NOTREACHED_NORETURN();
}

void BraveIPFSFallbackInfoBarDelegate::SetCheckboxChecked(bool checked) {
  NOTREACHED_IN_MIGRATION();
}

bool BraveIPFSFallbackInfoBarDelegate::InterceptClosing() {
  return false;
}

// ConfirmInfoBarDelegate
infobars::InfoBarDelegate::InfoBarIdentifier
BraveIPFSFallbackInfoBarDelegate::GetIdentifier() const {
  return BRAVE_IPFS_FALLBACK_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& BraveIPFSFallbackInfoBarDelegate::GetVectorIcon() const {
  return kLeoInfoOutlineIcon;
}

bool BraveIPFSFallbackInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return details.is_navigation_to_different_page;
}

void BraveIPFSFallbackInfoBarDelegate::InfoBarDismissed() {}

std::u16string BraveIPFSFallbackInfoBarDelegate::GetMessageText() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_IPFS_FALLBACK_INFOBAR_TEXT);
}

int BraveIPFSFallbackInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

std::vector<int> BraveIPFSFallbackInfoBarDelegate::GetButtonsOrder() const {
  return {InfoBarButton::BUTTON_OK, InfoBarButton::BUTTON_CANCEL};
}

std::u16string BraveIPFSFallbackInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_CANCEL) {
    return brave_l10n::GetLocalizedResourceUTF16String(
        IDS_BRAVE_IPFS_FALLBACK_INFOBAR_NO);
  }
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_IPFS_FALLBACK_REDIRECT);
}

bool BraveIPFSFallbackInfoBarDelegate::Accept() {
  if (observer_) {
    observer_->OnRedirectToOriginalAddress();
  }
  return true;
}

bool BraveIPFSFallbackInfoBarDelegate::Cancel() {
  return true;
}

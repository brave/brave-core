/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_ipfs_infobar_delegate.h"

#include <algorithm>
#include <utility>

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "ui/views/vector_icons.h"

// BraveIPFSInfoBarDelegateObserver
BraveIPFSInfoBarDelegateObserver::BraveIPFSInfoBarDelegateObserver() = default;

BraveIPFSInfoBarDelegateObserver::~BraveIPFSInfoBarDelegateObserver() = default;

// BraveIPFSInfoBarDelegate
// static
void BraveIPFSInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    std::unique_ptr<BraveIPFSInfoBarDelegateObserver> observer,
    PrefService* local_state) {
  if (!local_state->GetBoolean(kShowIPFSPromoInfobar)) {
    return;
  }
  infobar_manager->AddInfoBar(std::make_unique<BraveConfirmInfoBar>(
                                  std::make_unique<BraveIPFSInfoBarDelegate>(
                                      std::move(observer), local_state)),
                              true);
}

BraveIPFSInfoBarDelegate::BraveIPFSInfoBarDelegate(
    std::unique_ptr<BraveIPFSInfoBarDelegateObserver> observer,
    PrefService* local_state)
    : observer_(std::move(observer)), local_state_(local_state) {}

BraveIPFSInfoBarDelegate::~BraveIPFSInfoBarDelegate() {}

// BraveConfirmInfoBarDelegate
bool BraveIPFSInfoBarDelegate::HasCheckbox() const {
  return false;
}

std::u16string BraveIPFSInfoBarDelegate::GetCheckboxText() const {
  NOTREACHED_NORETURN();
}

void BraveIPFSInfoBarDelegate::SetCheckboxChecked(bool checked) {
  NOTREACHED_IN_MIGRATION();
}

bool BraveIPFSInfoBarDelegate::InterceptClosing() {
  return false;
}

// ConfirmInfoBarDelegate
infobars::InfoBarDelegate::InfoBarIdentifier
BraveIPFSInfoBarDelegate::GetIdentifier() const {
  return BRAVE_IPFS_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& BraveIPFSInfoBarDelegate::GetVectorIcon() const {
  return views::kInfoIcon;
}

bool BraveIPFSInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return details.is_navigation_to_different_page;
}

void BraveIPFSInfoBarDelegate::InfoBarDismissed() {}

std::u16string BraveIPFSInfoBarDelegate::GetMessageText() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_IPFS_INFOBAR_TEXT);
}

int BraveIPFSInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL | BUTTON_EXTRA;
}

bool BraveIPFSInfoBarDelegate::IsProminent(int id) const {
  return id == BUTTON_OK || id == BUTTON_EXTRA;
}

std::u16string BraveIPFSInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  switch (button) {
    case InfoBarButton::BUTTON_OK:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_IPFS_INFOBAR_APPROVE);
    case InfoBarButton::BUTTON_EXTRA:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_IPFS_INFOBAR_APPROVE_ONCE);
    case InfoBarButton::BUTTON_CANCEL:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_BRAVE_IPFS_INFOBAR_NEVER);
    default:
      NOTREACHED_NORETURN();
  }
}

std::vector<int> BraveIPFSInfoBarDelegate::GetButtonsOrder() const {
  return {InfoBarButton::BUTTON_OK, InfoBarButton::BUTTON_EXTRA,
          InfoBarButton::BUTTON_CANCEL};
}

std::u16string BraveIPFSInfoBarDelegate::GetLinkText() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_IPFS_INFOBAR_LINK);
}

GURL BraveIPFSInfoBarDelegate::GetLinkURL() const {
  return GURL(ipfs::kIPFSLearnMorePrivacyURL);
}

bool BraveIPFSInfoBarDelegate::Accept() {
  if (observer_) {
    local_state_->SetBoolean(kShowIPFSPromoInfobar, false);
    observer_->OnRedirectToIPFS(true);
  }
  return true;
}

bool BraveIPFSInfoBarDelegate::ExtraButtonPressed() {
  if (observer_) {
    observer_->OnRedirectToIPFS(false);
  }
  return true;
}

bool BraveIPFSInfoBarDelegate::Cancel() {
  local_state_->SetBoolean(kShowIPFSPromoInfobar, false);
  return true;
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_ipfs_always_start_infobar_delegate.h"

#include "base/memory/ptr_util.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/prefs/pref_service.h"

BraveIPFSAlwaysStartInfoBarDelegateFactory::
    BraveIPFSAlwaysStartInfoBarDelegateFactory(PrefService* local_state)
    : local_state_(local_state) {}

std::unique_ptr<BraveGlobalConfirmInfobarDelegate>
BraveIPFSAlwaysStartInfoBarDelegateFactory::Create() {
  if (!local_state_ || local_state_->GetBoolean(kIPFSAlwaysStartMode)) {
    return nullptr;
  }

  if (local_state_->GetBoolean(kIPFSAlwaysStartInfobarShown)) {
    return nullptr;
  }

  return base::WrapUnique(
      new BraveIPFSAlwaysStartInfoBarDelegate(local_state_));
}

infobars::InfoBarDelegate::InfoBarIdentifier
BraveIPFSAlwaysStartInfoBarDelegateFactory::GetInfoBarIdentifier() const {
  return BraveConfirmInfoBarDelegate::BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE;
}

BraveIPFSAlwaysStartInfoBarDelegate::BraveIPFSAlwaysStartInfoBarDelegate(
    PrefService* local_state)
    : local_state_(local_state) {}

BraveIPFSAlwaysStartInfoBarDelegate::~BraveIPFSAlwaysStartInfoBarDelegate() =
    default;

// ConfirmInfoBarDelegate
infobars::InfoBarDelegate::InfoBarIdentifier
BraveIPFSAlwaysStartInfoBarDelegate::GetIdentifier() const {
  return BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE;
}

std::u16string BraveIPFSAlwaysStartInfoBarDelegate::GetMessageText() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_IPFS_ALWAYS_START_INFOBAR_TEXT);
}

int BraveIPFSAlwaysStartInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

std::vector<int> BraveIPFSAlwaysStartInfoBarDelegate::GetButtonsOrder() const {
  return {BUTTON_OK, BUTTON_CANCEL};
}

std::u16string BraveIPFSAlwaysStartInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_CANCEL) {
    return brave_l10n::GetLocalizedResourceUTF16String(
        IDS_BRAVE_IPFS_ALWAYS_START_INFOBAR_NO);
  }
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_IPFS_ALWAYS_START_INFOBAR_OK);
}

bool BraveIPFSAlwaysStartInfoBarDelegate::Accept() {
  if (local_state_) {
    local_state_->SetBoolean(kIPFSAlwaysStartMode, true);
  }
  SetLastShownTime();
  return BraveGlobalConfirmInfobarDelegate::Accept();
}

bool BraveIPFSAlwaysStartInfoBarDelegate::Cancel() {
  SetLastShownTime();
  return BraveGlobalConfirmInfobarDelegate::Cancel();
}

void BraveIPFSAlwaysStartInfoBarDelegate::InfoBarDismissed() {
  SetLastShownTime();
  BraveGlobalConfirmInfobarDelegate::InfoBarDismissed();
}

void BraveIPFSAlwaysStartInfoBarDelegate::SetLastShownTime() {
  if (!local_state_) {
    return;
  }
  local_state_->SetBoolean(kIPFSAlwaysStartInfobarShown, true);
}

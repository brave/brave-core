/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_ipfs_always_start_infobar_delegate.h"

#include "base/functional/callback_helpers.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/prefs/pref_service.h"

BraveIPFSAlwaysStartInfoBarDelegateFactory::
    BraveIPFSAlwaysStartInfoBarDelegateFactory(ipfs::IpfsService* ipfs_service,
                                               PrefService* local_state)
    : local_state_(local_state), ipfs_service_(ipfs_service) {}

std::unique_ptr<BraveConfirmInfoBarDelegate>
BraveIPFSAlwaysStartInfoBarDelegateFactory::Create() {
  if (!local_state_ || local_state_->GetBoolean(kIPFSAlwaysStartMode) ||
      local_state_->GetInteger(kIPFSResolveMethod) !=
          static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL)) {
    return nullptr;
  }

  const base::Time last_shown_time =
      local_state_->GetTime(kIPFSAlwaysStartInfobarLastShowTime);
  const base::Time time_1_day_ago = base::Time::Now() - base::Days(1);
  if (!last_shown_time.is_null() && last_shown_time > time_1_day_ago) {
    return nullptr;
  }

  return base::WrapUnique(
      new BraveIPFSAlwaysStartInfoBarDelegate(ipfs_service_, local_state_));
}

infobars::InfoBarDelegate::InfoBarIdentifier
BraveIPFSAlwaysStartInfoBarDelegateFactory::GetInfoBarIdentifier() const {
  return BraveConfirmInfoBarDelegate::BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE;
}

BraveIPFSAlwaysStartInfoBarDelegate::BraveIPFSAlwaysStartInfoBarDelegate(
    ipfs::IpfsService* ipfs_service,
    PrefService* local_state)
    : local_state_(local_state), ipfs_service_(ipfs_service) {}

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
    if (!ipfs_service_->IsDaemonLaunched()) {
      ipfs_service_->LaunchDaemon(base::NullCallback());
    }
  }
  SetLastShownTime();
  return true;
}

bool BraveIPFSAlwaysStartInfoBarDelegate::Cancel() {
  SetLastShownTime();
  return true;
}

void BraveIPFSAlwaysStartInfoBarDelegate::InfoBarDismissed() {
  SetLastShownTime();
}

void BraveIPFSAlwaysStartInfoBarDelegate::SetLastShownTime() {
  if (!local_state_) {
    return;
  }
  local_state_->SetTime(kIPFSAlwaysStartInfobarLastShowTime, base::Time::Now());
}

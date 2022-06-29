/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_sync_account_deleted_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/grit/chromium_strings.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/views/vector_icons.h"

// static
void BraveSyncAccountDeletedInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    Profile* profile,
    Browser* browser) {
  brave_sync::Prefs brave_sync_prefs(profile->GetPrefs());
  const bool notification_pending =
      brave_sync_prefs.IsSyncAccountDeletedNoticePending();
  if (!notification_pending) {
    return;
  }

  // Show infobar
  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate>(
          new BraveSyncAccountDeletedInfoBarDelegate(browser, profile))));
}

// Start class impl
BraveSyncAccountDeletedInfoBarDelegate::BraveSyncAccountDeletedInfoBarDelegate(
    Browser* browser,
    Profile* profile)
    : ConfirmInfoBarDelegate(), profile_(profile), browser_(browser) {}

BraveSyncAccountDeletedInfoBarDelegate::
    ~BraveSyncAccountDeletedInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
BraveSyncAccountDeletedInfoBarDelegate::GetIdentifier() const {
  return BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR;
}

const gfx::VectorIcon& BraveSyncAccountDeletedInfoBarDelegate::GetVectorIcon()
    const {
  return views::kInfoIcon;
}

bool BraveSyncAccountDeletedInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return false;
}

void BraveSyncAccountDeletedInfoBarDelegate::InfoBarDismissed() {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.SetSyncAccountDeletedNoticePending(false);
}

std::u16string BraveSyncAccountDeletedInfoBarDelegate::GetMessageText() const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_MESSAGE);
}

int BraveSyncAccountDeletedInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

std::u16string BraveSyncAccountDeletedInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_COMMAND);
}

bool BraveSyncAccountDeletedInfoBarDelegate::Accept() {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.SetSyncAccountDeletedNoticePending(false);
  brave::ShowSync(browser_);
  return true;
}

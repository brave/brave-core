/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_sync_account_deleted_infobar_delegate.h"

#include <memory>

#include "brave/browser/ui/brave_pages.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "ui/views/vector_icons.h"

// static
void BraveSyncAccountDeletedInfoBarDelegate::Create(
    content::WebContents* active_web_contents,
    Profile* profile,
    Browser* browser) {
  brave_sync::Prefs brave_sync_prefs(profile->GetPrefs());
  const bool notification_pending =
      brave_sync_prefs.IsSyncAccountDeletedNoticePending();
  if (!notification_pending) {
    return;
  }

  // If we already are on brave://settings/braveSync/setup page, don't show
  // informer
  if (!active_web_contents || active_web_contents->GetURL() ==
                                  chrome::GetSettingsUrl(kBraveSyncSetupPath)) {
    return;
  }

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(active_web_contents);

  if (!infobar_manager) {
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

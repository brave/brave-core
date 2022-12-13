/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_sync_account_deleted_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/browser/ui/views/infobars/brave_sync_account_deleted_infobar.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"
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

  // Create custom confirm infobar
  std::unique_ptr<infobars::InfoBar> infobar(
      std::make_unique<BraveSyncAccountDeletedInfoBar>(
          base::WrapUnique<ConfirmInfoBarDelegate>(
              new BraveSyncAccountDeletedInfoBarDelegate(browser, profile))));

  // Show infobar
  infobar_manager->AddInfoBar(std::move(infobar));
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
  // The replacement with empty string here is required to eat placeholder $1
  // in grit string resource. And it's impossible to have empty placeholder
  // <ph name="NAME"></ph>, grit compiler gives error. Placeholder is required
  // to explane translation team that message string and link text are part of
  // the same sentense.
  return l10n_util::GetStringFUTF16(
      IDS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_MESSAGE, u"");
}

int BraveSyncAccountDeletedInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

std::u16string BraveSyncAccountDeletedInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_BUTTON);
}

std::u16string BraveSyncAccountDeletedInfoBarDelegate::GetLinkText() const {
  // See comment at |BraveSyncAccountDeletedInfoBarDelegate::GetMessageText|
  // above for empty substitution
  return l10n_util::GetStringFUTF16(
      IDS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_LINK_TEXT, u"");
}

GURL BraveSyncAccountDeletedInfoBarDelegate::GetLinkURL() const {
  return chrome::GetSettingsUrl(kBraveSyncSetupPath);
}

bool BraveSyncAccountDeletedInfoBarDelegate::Accept() {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.SetSyncAccountDeletedNoticePending(false);
  return true;
}

bool BraveSyncAccountDeletedInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.SetSyncAccountDeletedNoticePending(false);
  InfoBarDelegate::LinkClicked(disposition);
  return true;
}

bool BraveSyncAccountDeletedInfoBarDelegate::IsCloseable() const {
  return false;
}

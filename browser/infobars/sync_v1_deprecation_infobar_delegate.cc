/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/sync_v1_deprecation_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/grit/chromium_strings.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"

// static
void SyncV1DeprecationInfoBarDelegate::Create(InfoBarService* infobar_service,
                                            PrefService* prefs) {
  // Don't show infobar if:
  // - sync v1 flag is not enabled
  // - sync is disabled (pref, not flag)
  // - notice has already been dismissed / viewed
  const bool is_flag_enabled =
      base::FeatureList::IsEnabled(brave_sync::features::kBraveSync);
  if (!is_flag_enabled) {
    return;
  }
  if (!prefs->GetBoolean(brave_sync::prefs::kSyncEnabled) ||
      prefs->GetBoolean(
          brave_sync::prefs::kSyncDeprecationWarningNoticeDismissed)) {
    return;
  }

  // Only show the bar once, ever
  prefs->SetBoolean(
      brave_sync::prefs::kSyncDeprecationWarningNoticeDismissed, true);

  // Show it
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new SyncV1DeprecationInfoBarDelegate())));
}

SyncV1DeprecationInfoBarDelegate::SyncV1DeprecationInfoBarDelegate()
    : ConfirmInfoBarDelegate() {
}

SyncV1DeprecationInfoBarDelegate::~SyncV1DeprecationInfoBarDelegate() {
}

infobars::InfoBarDelegate::InfoBarIdentifier
SyncV1DeprecationInfoBarDelegate::GetIdentifier() const {
  return SYNC_V1_DEPRECATION_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& SyncV1DeprecationInfoBarDelegate::GetVectorIcon() const {
  return vector_icons::kSyncIcon;
}

int SyncV1DeprecationInfoBarDelegate::GetButtons() const {
  return BUTTON_NONE;
}

base::string16 SyncV1DeprecationInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(
      IDS_BRAVE_SYNC_V1_DEPRECATED_INFOBAR_MESSAGE);
}

base::string16 SyncV1DeprecationInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL SyncV1DeprecationInfoBarDelegate::GetLinkURL() const {
  return GURL(kSyncV1DeprecatedURL);
}

bool SyncV1DeprecationInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  // Open the link, but also close the infobar
  ConfirmInfoBarDelegate::LinkClicked(disposition);
  return true;
}

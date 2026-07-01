/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/safe_browsing/brave_download_protection_delegate_desktop.h"

#include "brave/components/safebrowsing/constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/common/download_item.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/file_system_access_write_item.h"

namespace safe_browsing {

namespace {

bool IsBraveDownloadProtectionEnabled(Profile* profile) {
  return profile && profile->GetPrefs()->GetBoolean(
                        kBraveSafeBrowsingDownloadProtectionEnabled);
}

Profile* GetProfile(download::DownloadItem* item) {
  return Profile::FromBrowserContext(
      content::DownloadItemUtils::GetBrowserContext(item));
}

}  // namespace

bool BraveDownloadProtectionDelegateDesktop::ShouldCheckDownloadUrl(
    download::DownloadItem* item) const {
  return IsBraveDownloadProtectionEnabled(GetProfile(item)) &&
         DownloadProtectionDelegateDesktop::ShouldCheckDownloadUrl(item);
}

bool BraveDownloadProtectionDelegateDesktop::MayCheckClientDownload(
    download::DownloadItem* item) const {
  return IsBraveDownloadProtectionEnabled(GetProfile(item)) &&
         DownloadProtectionDelegateDesktop::MayCheckClientDownload(item);
}

bool BraveDownloadProtectionDelegateDesktop::MayCheckFileSystemAccessWrite(
    content::FileSystemAccessWriteItem* item) const {
  Profile* profile = Profile::FromBrowserContext(item->browser_context);
  return IsBraveDownloadProtectionEnabled(profile) &&
         DownloadProtectionDelegateDesktop::MayCheckFileSystemAccessWrite(item);
}

}  // namespace safe_browsing

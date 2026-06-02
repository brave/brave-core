/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/safe_browsing/download_protection/download_protection_delegate.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/components/safebrowsing/constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_delegate_desktop.h"
#include "components/download/public/common/download_item.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/file_system_access_write_item.h"

namespace safe_browsing {
namespace {

bool IsBraveDownloadProtectionEnabled(Profile* profile) {
  return profile && profile->GetPrefs()->GetBoolean(
                        kBraveSafeBrowsingDownloadProtectionEnabled);
}

}  // namespace

// Gates the desktop download-protection checks behind Brave's separate
// download-protection pref.
class BraveDownloadProtectionDelegateDesktop
    : public DownloadProtectionDelegateDesktop {
 public:
  BraveDownloadProtectionDelegateDesktop() = default;
  ~BraveDownloadProtectionDelegateDesktop() override = default;

  bool ShouldCheckDownloadUrl(download::DownloadItem* item) const override {
    return IsBraveDownloadProtectionEnabled(GetProfile(item)) &&
           DownloadProtectionDelegateDesktop::ShouldCheckDownloadUrl(item);
  }

  bool MayCheckClientDownload(download::DownloadItem* item) const override {
    return IsBraveDownloadProtectionEnabled(GetProfile(item)) &&
           DownloadProtectionDelegateDesktop::MayCheckClientDownload(item);
  }

  bool MayCheckFileSystemAccessWrite(
      content::FileSystemAccessWriteItem* item) const override {
    Profile* profile = Profile::FromBrowserContext(item->browser_context);
    return IsBraveDownloadProtectionEnabled(profile) &&
           DownloadProtectionDelegateDesktop::MayCheckFileSystemAccessWrite(
               item);
  }

 private:
  static Profile* GetProfile(download::DownloadItem* item) {
    return Profile::FromBrowserContext(
        content::DownloadItemUtils::GetBrowserContext(item));
  }
};

}  // namespace safe_browsing

#define DownloadProtectionDelegateDesktop BraveDownloadProtectionDelegateDesktop
#endif  // !BUILDFLAG(IS_ANDROID)

#include <chrome/browser/safe_browsing/download_protection/download_protection_delegate.cc>

#if !BUILDFLAG(IS_ANDROID)
#undef DownloadProtectionDelegateDesktop
#endif

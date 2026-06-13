/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SAFE_BROWSING_BRAVE_DOWNLOAD_PROTECTION_DELEGATE_DESKTOP_H_
#define BRAVE_BROWSER_SAFE_BROWSING_BRAVE_DOWNLOAD_PROTECTION_DELEGATE_DESKTOP_H_

#include "chrome/browser/safe_browsing/download_protection/download_protection_delegate_desktop.h"

namespace content {
struct FileSystemAccessWriteItem;
}  // namespace content

namespace download {
class DownloadItem;
}  // namespace download

namespace safe_browsing {

// Gates the desktop download-protection checks behind Brave's separate
// download-protection pref (kBraveSafeBrowsingDownloadProtectionEnabled). Each
// override returns false when the pref is off; otherwise it defers to the
// upstream desktop delegate.
class BraveDownloadProtectionDelegateDesktop
    : public DownloadProtectionDelegateDesktop {
 public:
  BraveDownloadProtectionDelegateDesktop() = default;
  ~BraveDownloadProtectionDelegateDesktop() override = default;

  // DownloadProtectionDelegateDesktop:
  bool ShouldCheckDownloadUrl(download::DownloadItem* item) const override;
  bool MayCheckClientDownload(download::DownloadItem* item) const override;
  bool MayCheckFileSystemAccessWrite(
      content::FileSystemAccessWriteItem* item) const override;
};

}  // namespace safe_browsing

#endif  // BRAVE_BROWSER_SAFE_BROWSING_BRAVE_DOWNLOAD_PROTECTION_DELEGATE_DESKTOP_H_

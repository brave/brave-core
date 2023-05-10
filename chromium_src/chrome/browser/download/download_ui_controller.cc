/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "chrome/browser/download/bubble/download_bubble_prefs.h"
#include "chrome/browser/download/download_stats.h"

namespace download {
bool BraveIsDownloadBubbleV2Enabled(Profile* profile);
}  // namespace download

bool IsIpfsImportDownloadCreated(content::WebContents* web_contents,
                                 download::DownloadItem* item);

#define RecordDownloadStartPerProfileType                \
  if (IsIpfsImportDownloadCreated(web_contents, item)) { \
    return;                                              \
  }                                                      \
  RecordDownloadStartPerProfileType

// Prevent DownloadBubbleUIControllerDelegate from overriding
// prefs::kPromptForDownload value for OffTheRecord profiles
#define IsDownloadBubbleV2Enabled BraveIsDownloadBubbleV2Enabled

#include "src/chrome/browser/download/download_ui_controller.cc"
#undef IsDownloadBubbleV2Enabled
#undef RecordDownloadStartPerProfileType

bool IsIpfsImportDownloadCreated(content::WebContents* web_contents,
                                 download::DownloadItem* item) {
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(web_contents);
  return (helper && helper->GetImportController()->HasInProgressDownload(item));
}

namespace download {

bool BraveIsDownloadBubbleV2Enabled(Profile* profile) {
  return false;
}

}  // namespace download

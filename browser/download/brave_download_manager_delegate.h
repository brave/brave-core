/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_MANAGER_DELEGATE_H_
#define BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_MANAGER_DELEGATE_H_

#include "chrome/browser/download/chrome_download_manager_delegate.h"

class Profile;

class BraveDownloadManagerDelegate : public ChromeDownloadManagerDelegate {
 public:
  explicit BraveDownloadManagerDelegate(Profile* profile);
  BraveDownloadManagerDelegate(const BraveDownloadManagerDelegate&) = delete;
  BraveDownloadManagerDelegate& operator=(const BraveDownloadManagerDelegate&) =
      delete;
  ~BraveDownloadManagerDelegate() override;

 private:
  // ChromeDownloadManagerDelegate:
  bool IsDownloadReadyForCompletion(
      download::DownloadItem* item,
      base::OnceClosure internal_complete_callback) override;
};

#endif  // BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_MANAGER_DELEGATE_H_

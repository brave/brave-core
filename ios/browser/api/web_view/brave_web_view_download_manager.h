// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_DOWNLOAD_MANAGER_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_DOWNLOAD_MANAGER_H_

#include "ios/web_view/internal/web_view_download_manager.h"

class BraveWebViewDownloadManager
    : public ios_web_view::WebViewDownloadManager {
  using ios_web_view::WebViewDownloadManager::WebViewDownloadManager;
  void OnDownloadCreated(web::DownloadController* download_controller,
                         web::WebState* web_state,
                         std::unique_ptr<web::DownloadTask> task) override;
};

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_DOWNLOAD_MANAGER_H_

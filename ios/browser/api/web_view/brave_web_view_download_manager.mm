// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view_download_manager.h"

#include "brave/ios/browser/api/web_view/brave_web_view_internal.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web_view/internal/web_view_download_manager.h"

void BraveWebViewDownloadManager::OnDownloadCreated(
    web::DownloadController* controller,
    web::WebState* web_state,
    std::unique_ptr<web::DownloadTask> task) {
  ios_web_view::WebViewDownloadManager::OnDownloadCreated(controller, web_state,
                                                          std::move(task));
  __weak BraveWebView* web_view =
      [BraveWebView braveWebViewForWebState:web_state];
  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(^{
        // The URLs are not always updated in time after the download is created
        // so this attempts to update them on next runloop
        [web_view updateForOnDownloadCreated];
      }));
}

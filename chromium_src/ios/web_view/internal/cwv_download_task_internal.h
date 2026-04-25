// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_DOWNLOAD_TASK_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_DOWNLOAD_TASK_INTERNAL_H_

// This override just exposes the internally held web::DownloadTask as a
// property so that we can add additional methods to CWVDownloadTask that use
// it such as exposing the originating host

namespace web {
class DownloadTask;
}

#include <ios/web_view/internal/cwv_download_task_internal.h>  // IWYU pragma: export

@interface CWVDownloadTask (Internal)
@property(readonly) web::DownloadTask* internalTask;
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_DOWNLOAD_TASK_INTERNAL_H_

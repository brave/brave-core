// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/public/download/download_task.h"

#include <ios/web_view/internal/cwv_download_task.mm>

@implementation CWVDownloadTask (Internal)

- (web::DownloadTask*)internalTask {
  return _internalTask.get();
}

@end

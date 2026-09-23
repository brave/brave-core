// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_download_task_extras.h"

#include "ios/web/public/download/download_task.h"
#include "ios/web_view/internal/cwv_download_task_internal.h"
#include "net/base/apple/url_conversions.h"

@implementation CWVDownloadTask (Extras)

- (NSString*)originatingHost {
  return self.internalTask->GetOriginatingHost();
}

- (NSURL*)redirectedURL {
  return net::NSURLWithGURL(self.internalTask->GetRedirectedUrl());
}

@end

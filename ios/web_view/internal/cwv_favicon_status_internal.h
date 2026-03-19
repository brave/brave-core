// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_VIEW_INTERNAL_CWV_FAVICON_STATUS_INTERNAL_H_
#define BRAVE_IOS_WEB_VIEW_INTERNAL_CWV_FAVICON_STATUS_INTERNAL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/web_view/public/cwv_favicon_status.h"

namespace web {
struct FaviconStatus;
}

NS_ASSUME_NONNULL_BEGIN

@interface CWVFaviconStatus (Internal)
- (instancetype)initWithFaviconStatus:(web::FaviconStatus)status;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_INTERNAL_CWV_FAVICON_STATUS_INTERNAL_H_

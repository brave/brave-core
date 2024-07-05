// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_WEB_VIEW_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_WEB_VIEW_INTERNAL_H_

#include "src/ios/web_view/internal/cwv_web_view_internal.h"  // IWYU pragma: export

NS_ASSUME_NONNULL_BEGIN

@interface CWVWebView (Internal)
@property(readonly) web::WebState* webState;
+ (BOOL)_isRestoreDataValid:(NSData*)data;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_WEB_VIEW_INTERNAL_H_

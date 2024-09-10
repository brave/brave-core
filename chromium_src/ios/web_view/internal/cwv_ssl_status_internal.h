// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_SSL_STATUS_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_SSL_STATUS_INTERNAL_H_

#include "src/ios/web_view/internal/cwv_ssl_status_internal.h"  // IWYU pragma: export

@interface CWVSSLStatus (Internal)
@property(readonly) web::SSLStatus internalStatus;
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_CWV_SSL_STATUS_INTERNAL_H_

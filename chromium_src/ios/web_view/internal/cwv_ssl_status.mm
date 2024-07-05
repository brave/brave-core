// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/ios/web_view/internal/cwv_ssl_status.mm"

@implementation CWVSSLStatus (Internal)
- (web::SSLStatus)internalStatus {
  return _internalStatus;
}
@end

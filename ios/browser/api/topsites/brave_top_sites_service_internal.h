// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_NTP_BRAVE_TOP_SITES_SERVICE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_NTP_BRAVE_TOP_SITES_SERVICE_PRIVATE_H_

#import "brave/ios/browser/api/topsites/brave_top_sites_service.h"

class ProfileIOS;

@interface BraveTopSitesService (Private)
- (instancetype)initWithProfile:(ProfileIOS*)profile;
@end

#endif

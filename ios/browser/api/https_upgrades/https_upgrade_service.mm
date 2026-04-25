// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/https_upgrades/https_upgrade_service.h"

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "ios/components/security_interstitials/https_only_mode/https_upgrade_service.h"

@implementation BraveHttpsUpgradeService {
  raw_ptr<HttpsUpgradeService> _service;
}

- (instancetype)initWithHttpsUpgradeService:(HttpsUpgradeService*)service {
  if ((self = [super init])) {
    _service = service;
  }
  return self;
}

- (bool)isHttpAllowedForHost:(NSString*)host {
  return _service->IsHttpAllowedForHost(base::SysNSStringToUTF8(host));
}

- (void)allowHttpForHost:(NSString*)host {
  _service->AllowHttpForHost(base::SysNSStringToUTF8(host));
}

- (void)clearAllowlistFromStartDate:(NSDate*)startDate
                            endDate:(NSDate*)endDate {
  _service->ClearAllowlist(base::Time::FromNSDate(startDate),
                           base::Time::FromNSDate(endDate));
}

@end

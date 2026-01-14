// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_origin/brave_origin_service_bridge_impl.h"

#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/ios/browser/brave_origin/brave_origin_service_bridge.h"

@interface BraveOriginServiceBridgeImpl () {
  raw_ptr<brave_origin::BraveOriginService> _service;
}
@end

@implementation BraveOriginServiceBridgeImpl

- (instancetype)initWithBraveOriginService:
    (brave_origin::BraveOriginService*)service {
  if ((self = [super init])) {
    _service = service;
  }
  return self;
}

- (BOOL)isPolicyControlledByBraveOrigin:(BraveOriginPolicyKey)policyKey {
  return _service->IsPolicyControlledByBraveOrigin(
      base::SysNSStringToUTF8(policyKey));
}

- (BOOL)setPolicyValue:(BraveOriginPolicyKey)policyKey value:(BOOL)value {
  return _service->SetPolicyValue(base::SysNSStringToUTF8(policyKey), value);
}

- (NSNumber*)getPolicyValue:(BraveOriginPolicyKey)policyKey {
  std::optional<bool> result =
      _service->GetPolicyValue(base::SysNSStringToUTF8(policyKey));
  if (result.has_value()) {
    return @(result.value());
  }
  return nil;
}

@end

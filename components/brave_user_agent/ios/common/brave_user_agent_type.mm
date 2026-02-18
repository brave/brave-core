// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/components/brave_user_agent/ios/common/brave_user_agent_type.h"

#import <Foundation/Foundation.h>

#include "brave/components/brave_user_agent/common/features.h"

BraveIOSUserAgentType GetDefaultBraveIOSUserAgentType() {
  if (!base::FeatureList::IsEnabled(
          brave_user_agent::features::kUseBraveUserAgent)) {
    // Feature is disabled, use masked / Safari user agent.
    return BraveIOSUserAgentTypeMasked;
  }
  // Feature is enabled, get the feature param value to determine which user
  // agent type to use
  int raw_value = brave_user_agent::features::kBraveIOSUserAgentDefault.Get();
  if (raw_value >= BraveIOSUserAgentTypeMasked &&
      raw_value <= BraveIOSUserAgentTypeSuffixComment) {
    return static_cast<BraveIOSUserAgentType>(raw_value);
  }
  // if unable to determine value, but feature is enabled, default to Suffix.
  return BraveIOSUserAgentTypeSuffix;
}

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_shields/ios/browser/brave_shields_settings_bridge.h"
#include "brave/components/brave_shields/ios/common/shields_settings.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

namespace brave_shields {
class BraveShieldsSettingsService;
}

@interface BraveShieldsSettingsBridgeImpl
    : NSObject <BraveShieldsSettingsBridge>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithBraveShieldsSettings:
    (raw_ptr<brave_shields::BraveShieldsSettingsService>)braveShieldsSettings;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_IMPL_H_

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol BraveShieldsSettingsBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(BraveShieldsSettingsServiceFactory)
@interface BraveShieldsSettingsServiceFactoryBridge
    : KeyedServiceFactoryWrapper <id <BraveShieldsSettingsBridge>>
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_SERVICE_FACTORY_BRIDGE_H_

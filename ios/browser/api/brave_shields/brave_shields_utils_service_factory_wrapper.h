/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_SERVICE_SERVICE_FACTORY_WRAPPER_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_SERVICE_SERVICE_FACTORY_WRAPPER_H_

#import <Foundation/Foundation.h>

#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol BraveShieldsBraveShieldsUtilsService;

OBJC_EXPORT
NS_SWIFT_NAME(BraveShields.ServiceFactory)
@interface BraveShieldsUtilsServiceFactory
    : KeyedServiceFactoryWrapper <id <BraveShieldsBraveShieldsUtilsService>>
@end

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_SERVICE_SERVICE_FACTORY_WRAPPER_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_FACTORY_WRAPPERS_H_
#define BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_FACTORY_WRAPPERS_H_

#import <Foundation/Foundation.h>
#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol SkusSkusSdk;

OBJC_EXPORT
NS_SWIFT_NAME(Skus.SkusSkusSdk)
@interface SkusSkusSdkFactory : KeyedServiceFactoryWrapper<id<SkusSkusSdk>>
@end

#endif  // BRAVE_IOS_BROWSER_SKUS_SKUS_SDK_FACTORY_WRAPPERS_H_
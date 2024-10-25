/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_KEYED_SERVICE_KEYED_SERVICE_FACTORY_WRAPPER_PRIVATE_H_
#define BRAVE_IOS_BROWSER_KEYED_SERVICE_KEYED_SERVICE_FACTORY_WRAPPER_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper.h"

class ProfileIOS;

NS_ASSUME_NONNULL_BEGIN

@interface KeyedServiceFactoryWrapper (Private)

+ (nullable id)serviceForProfile:(ProfileIOS*)profile;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_KEYED_SERVICE_KEYED_SERVICE_FACTORY_WRAPPER_PRIVATE_H_

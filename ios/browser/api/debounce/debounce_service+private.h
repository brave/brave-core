// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_DEBOUNCE_DEBOUNCE_SERVICE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_DEBOUNCE_DEBOUNCE_SERVICE_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/debounce/debounce_service.h"

namespace debounce {
class DebounceService;
}

NS_ASSUME_NONNULL_BEGIN

@interface DebounceService (Private)

- (instancetype)initWithDebounceService:
    (debounce::DebounceService*)debounceService;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_DEBOUNCE_DEBOUNCE_SERVICE_PRIVATE_H_

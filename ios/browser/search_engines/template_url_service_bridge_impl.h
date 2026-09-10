// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/search_engines/template_url_service_bridge.h"

class PrefService;
class TemplateURLService;

NS_ASSUME_NONNULL_BEGIN

@interface TemplateURLServiceBridgeImpl : NSObject <TemplateURLServiceBridge>

- (instancetype)init NS_UNAVAILABLE;

/// `service` must outlive this object. `prefs` is the profile's pref service,
/// used to read/write the private-mode default search engine.
- (instancetype)initWithTemplateURLService:(TemplateURLService*)service
                                     prefs:(PrefService*)prefs
    NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_BRIDGE_IMPL_H_

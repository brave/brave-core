// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_FACTORY_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_FACTORY_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol TemplateURLServiceBridge;

NS_ASSUME_NONNULL_BEGIN

/// Factory for obtaining the `TemplateURLServiceBridge` for a profile. Both
/// regular and private browsing return the regular profile's service; use
/// `TemplateURLServiceBridge.defaultPrivateSearchProvider` and
/// `setUserSelectedDefaultPrivateSearchProviderWithGUID:` for private-mode
/// defaults.
OBJC_EXPORT
NS_SWIFT_NAME(TemplateURLServiceFactory)
@interface TemplateURLServiceFactoryBridge
    : KeyedServiceFactoryWrapper <id <TemplateURLServiceBridge>>
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_FACTORY_BRIDGE_H_

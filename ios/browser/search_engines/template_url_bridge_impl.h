// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/search_engines/template_url_bridge.h"

class TemplateURL;

NS_ASSUME_NONNULL_BEGIN

@interface TemplateURLBridge ()

- (instancetype)initWithTemplateURL:(const TemplateURL*)templateURL;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_IMPL_H_

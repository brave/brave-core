// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/search_engines/template_url_bridge.h"

class SearchTermsData;
class TemplateURL;
class TemplateURLRef;

NS_ASSUME_NONNULL_BEGIN

@interface TemplateURLBridge ()

@property(readonly) const TemplateURL* templateURL;

- (instancetype)initWithTemplateURL:(const TemplateURL*)templateURL
    NS_DESIGNATED_INITIALIZER;

@end

@interface TemplateURLRefBridge ()

/// `templateURLRef` is owned by a `TemplateURL` and has the same lifetime
/// requirements as `TemplateURLBridge`.
- (instancetype)initWithTemplateURLRef:(const TemplateURLRef*)templateURLRef
    NS_DESIGNATED_INITIALIZER;

@end

@interface SearchTermsDataBridge ()

@property(readonly) const SearchTermsData* searchTermsData;

/// `searchTermsData` must outlive this object.
- (instancetype)initWithSearchTermsData:(const SearchTermsData*)searchTermsData
    NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_BRIDGE_IMPL_H_

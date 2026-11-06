// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/omnibox/autocomplete_classifier_bridge.h"

class AutocompleteClassifier;

NS_ASSUME_NONNULL_BEGIN

@interface AutocompleteClassifierBridgeImpl
    : NSObject <AutocompleteClassifierBridge>
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithAutocompleteClassifier:
    (AutocompleteClassifier*)classifier NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_OMNIBOX_AUTOCOMPLETE_CLASSIFIER_BRIDGE_IMPL_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/omnibox/autocomplete_classifier_factory_bridge.h"

#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "brave/ios/browser/omnibox/autocomplete_classifier_bridge_impl.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "ios/chrome/browser/autocomplete/model/autocomplete_classifier_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation AutocompleteClassifierFactoryBridge

+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  AutocompleteClassifier* classifier =
      ios::AutocompleteClassifierFactory::GetForProfile(profile);
  if (!classifier) {
    return nil;
  }
  return [[AutocompleteClassifierBridgeImpl alloc]
      initWithAutocompleteClassifier:classifier];
}

@end

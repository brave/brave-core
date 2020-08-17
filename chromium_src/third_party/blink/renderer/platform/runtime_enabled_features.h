/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_RUNTIME_ENABLED_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_RUNTIME_ENABLED_FEATURES_H_

#define RuntimeEnabledFeatures RuntimeEnabledFeaturesBase
#include "../gen/third_party/blink/renderer/platform/runtime_enabled_features.h"
#undef RuntimeEnabledFeatures

namespace blink {

class PLATFORM_EXPORT RuntimeEnabledFeatures
    : public RuntimeEnabledFeaturesBase {
  STATIC_ONLY(RuntimeEnabledFeatures);

 public:
  // Hide these base class static methods:
  static bool SignedExchangePrefetchCacheForNavigationsEnabled() {
    return false;
  }
  static bool SignedExchangePrefetchCacheForNavigationsEnabled(
      const FeatureContext*) {
    return false;
  }
  static bool SignedExchangeSubresourcePrefetchEnabledByRuntimeFlag() {
    return false;
  }
  static bool SignedExchangeSubresourcePrefetchEnabled(const FeatureContext*) {
    return false;
  }
  static bool SubresourceWebBundlesEnabled() { return false; }
  static bool SubresourceWebBundlesEnabled(const FeatureContext*) {
    return false;
  }
};

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_RUNTIME_ENABLED_FEATURES_H_

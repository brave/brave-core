// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {
namespace {

class RuntimeEnabledFeaturesTest : public testing::Test {
 public:
  void CheckAllDisabled() {
    EXPECT_FALSE(RuntimeEnabledFeatures::
                     SignedExchangePrefetchCacheForNavigationsEnabled());
    EXPECT_FALSE(RuntimeEnabledFeatures::
                     SignedExchangePrefetchCacheForNavigationsEnabled(nullptr));
    EXPECT_FALSE(RuntimeEnabledFeatures::
                     SignedExchangeSubresourcePrefetchEnabledByRuntimeFlag());
    EXPECT_FALSE(
        RuntimeEnabledFeatures::SignedExchangeSubresourcePrefetchEnabled(
            nullptr));
    EXPECT_FALSE(RuntimeEnabledFeatures::SubresourceWebBundlesEnabled());
    EXPECT_FALSE(RuntimeEnabledFeatures::SubresourceWebBundlesEnabled(nullptr));
  }

  void CheckChromiumImplState(bool state) {
    EXPECT_EQ(state, RuntimeEnabledFeaturesBase::
                     SignedExchangePrefetchCacheForNavigationsEnabled());
    EXPECT_EQ(state, RuntimeEnabledFeaturesBase::
                     SignedExchangePrefetchCacheForNavigationsEnabled(nullptr));
    EXPECT_EQ(state, RuntimeEnabledFeaturesBase::
                     SignedExchangeSubresourcePrefetchEnabledByRuntimeFlag());
    EXPECT_EQ(
        state,
        RuntimeEnabledFeaturesBase::SignedExchangeSubresourcePrefetchEnabled(
            nullptr));
    EXPECT_EQ(state,
              RuntimeEnabledFeaturesBase::SubresourceWebBundlesEnabled());
    EXPECT_EQ(state, RuntimeEnabledFeaturesBase::SubresourceWebBundlesEnabled(
                         nullptr));
  }

  void CheckChromiumImplDisabled() { CheckChromiumImplState(false); }
  void CheckChromiumImplEnabled() { CheckChromiumImplState(true); }

  void SetFeaturesState(bool state) {
    RuntimeEnabledFeatures::SetSignedExchangePrefetchCacheForNavigationsEnabled(
        state);
    RuntimeEnabledFeatures::SetSignedExchangeSubresourcePrefetchEnabled(state);
    RuntimeEnabledFeatures::SetSubresourceWebBundlesEnabled(state);
  }

  void SetFeaturesDisabled() { SetFeaturesState(false); }
  void SetFeaturesEnabled() { SetFeaturesState(true); }
};

TEST_F(RuntimeEnabledFeaturesTest, TestSXGandWebBundlesDisabled) {
  // Initial state - all disabled.
  CheckChromiumImplDisabled();
  CheckAllDisabled();

  // Enable SXG and WebBundles
  SetFeaturesEnabled();
  // Check that features are enabled via the chromium implementation
  CheckChromiumImplEnabled();
  // Should all come back as disabled.
  CheckAllDisabled();

  // Reset to disabled.
  SetFeaturesDisabled();
  CheckChromiumImplDisabled();
  CheckAllDisabled();

  // Enable using SetFeatureEnabledFromString API.
  RuntimeEnabledFeatures::SetFeatureEnabledFromString(
      "SignedExchangePrefetchCacheForNavigations", true);
  RuntimeEnabledFeatures::SetFeatureEnabledFromString(
      "SignedExchangeSubresourcePrefetch", true);
  RuntimeEnabledFeatures::SetFeatureEnabledFromString("SubresourceWebBundles",
                                                      true);
  // Check that features are enabled via the chromium implementation
  CheckChromiumImplEnabled();
  // Should all come back as disabled.
  CheckAllDisabled();
}

}  // namespace
}  // namespace blink

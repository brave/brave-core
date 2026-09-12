/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_runtime_features.h"

namespace {

constexpr auto kDisabledRuntimeFeatures =
    base::MakeFixedFlatSet<std::string_view>({
        "AdInterestGroupAPI",
        "AIPromptAPI",
        "AIPromptAPIMultimodalInput",
        "AIProofreadingAPI",
        "AIRewriterAPI",
        "AISummarizationAPI",
        "AIWriterAPI",
        "ControlledFrame",
        "FencedFrames",
        "Fledge",
        "LanguageDetectionAPI",
        "Parakeet",
        "Prerender2",
        "TranslationAPI",
        "UserMediaElement",
    });

constexpr auto kEnabledRuntimeFeatures =
    base::MakeFixedFlatSet<std::string_view>({
        "ReduceUserAgentMinorVersion",
    });

bool IsEnabled(std::string_view name) {
  return blink::WebRuntimeFeatures::IsFeatureEnabledFromStringForTesting(name);
}

void ExpectTheForcedState() {
  for (std::string_view name : kDisabledRuntimeFeatures) {
    EXPECT_FALSE(IsEnabled(name)) << name;
  }
  for (std::string_view name : kEnabledRuntimeFeatures) {
    EXPECT_TRUE(IsEnabled(name)) << name;
  }
}

TEST(BlinkRuntimeFeatureDefaultsTest, ForcedRuntimeFeatureDefaults) {
  ExpectTheForcedState();
}

TEST(BlinkRuntimeFeatureDefaultsTest, BaseFeatureSyncPreservesTheDefaults) {
  blink::WebRuntimeFeatures::UpdateStatusFromBaseFeatures();

  ExpectTheForcedState();
}

}  // namespace

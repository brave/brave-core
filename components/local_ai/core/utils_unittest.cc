/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

using Quality = media::mojom::SpeechRecognitionQuality;

class LocalAIUtilsUnitTest : public testing::Test {
 protected:
  base::test::ScopedFeatureList feature_list_;
};

// Tests the two qualities Brave's own model serves.
TEST_F(LocalAIUtilsUnitTest, ServedQualities) {
  feature_list_.InitAndEnableFeature(kBraveOnDeviceSpeechRecognition);

  EXPECT_TRUE(IsQualityServedByBraveOnDeviceSpeech(Quality::kCommand));
  EXPECT_TRUE(IsQualityServedByBraveOnDeviceSpeech(Quality::kDictation));
  EXPECT_FALSE(IsQualityServedByBraveOnDeviceSpeech(Quality::kConversation));
}

// Tests that with the feature off no quality is served, because Brave then has
// no on-device backend at all.
TEST_F(LocalAIUtilsUnitTest, NoQualityServedWhenFeatureDisabled) {
  feature_list_.InitAndDisableFeature(kBraveOnDeviceSpeechRecognition);

  EXPECT_FALSE(IsQualityServedByBraveOnDeviceSpeech(Quality::kCommand));
  EXPECT_FALSE(IsQualityServedByBraveOnDeviceSpeech(Quality::kDictation));
  EXPECT_FALSE(IsQualityServedByBraveOnDeviceSpeech(Quality::kConversation));
}

}  // namespace local_ai

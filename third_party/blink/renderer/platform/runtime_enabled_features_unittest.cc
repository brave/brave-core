/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

#include <initializer_list>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_runtime_features.h"

namespace blink {

namespace {

struct RuntimeFlag {
  const char* name;
  bool (*is_enabled)(const FeatureContext*);
};

// Refers to the generated accessor, so a flag upstream removes or renames
// fails the build instead of leaving a stale entry behind.
#define RUNTIME_FLAG(name) {#name, &RuntimeEnabledFeatures::name##Enabled}

// The flags start at what their `status` says, which for most of them is
// on, so they pass only if `UpdateStatusFromBaseFeatures()`, as run at
// renderer startup, copies Brave's `base::Feature` state into them.
void ExpectRuntimeFlagsFollowBaseFeatures(
    std::initializer_list<RuntimeFlag> flags,
    bool enabled) {
  RuntimeEnabledFeatures::Backup backup;
  WebRuntimeFeatures::UpdateStatusFromBaseFeatures();

  for (const auto& flag : flags) {
    SCOPED_TRACE(flag.name);
    EXPECT_EQ(flag.is_enabled(nullptr), enabled);
  }
  backup.Restore();
}

}  // namespace

// These match the `set_blink_runtime_enabled_feature_state` entries in
// brave/rewrite/third_party/blink/renderer/platform/
// runtime_enabled_features.json5.yaml.
TEST(BraveRuntimeEnabledFeaturesTest, DisabledFlags) {
  // Please, keep alphabetized.
  ExpectRuntimeFlagsFollowBaseFeatures(
      {
          RUNTIME_FLAG(AIEmbeddingsAPI),
          RUNTIME_FLAG(AIPromptAPI),
          RUNTIME_FLAG(AIPromptAPIMultimodalInput),
          RUNTIME_FLAG(AIProofreadingAPI),
          RUNTIME_FLAG(AIRewriterAPI),
          RUNTIME_FLAG(AISummarizationAPI),
          RUNTIME_FLAG(AIWriterAPI),
          RUNTIME_FLAG(ControlledFrame),
          RUNTIME_FLAG(ExtensionScriptTagging),
          RUNTIME_FLAG(FencedFrames),
          RUNTIME_FLAG(LanguageDetectionAPI),
          RUNTIME_FLAG(Prerender2),
          RUNTIME_FLAG(TranslationAPI),
          RUNTIME_FLAG(UserMediaElement),
      },
      false);
}

TEST(BraveRuntimeEnabledFeaturesTest, EnabledFlags) {
  // Please, keep alphabetized.
  ExpectRuntimeFlagsFollowBaseFeatures(
      {
          RUNTIME_FLAG(ReduceUserAgentMinorVersion),
      },
      true);
}

#undef RUNTIME_FLAG

}  // namespace blink

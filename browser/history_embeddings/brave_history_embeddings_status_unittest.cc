/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/history_embeddings/brave_history_embeddings_status.h"

#include <memory>
#include <utility>

#include "base/test/scoped_feature_list.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace history_embeddings {

// The setting is captured at profile setup, so each test picks the value the
// embedding services would have been built with before building `profile_`.
// `feature_list_` is a member so the feature is already on by then — otherwise
// the status would read off for two reasons at once.
class BraveHistoryEmbeddingsStatusTest : public testing::Test {
 protected:
  void BuildProfileWithSetting(bool enabled) {
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    prefs->SetBoolean(local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);

    TestingProfile::Builder builder;
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();

    // Stands in for BraveProfileManager::InitProfileUserPrefs(), which
    // TestingProfile skips without a TestingProfileManager.
    BraveHistoryEmbeddingsStatus::CreateForProfile(profile_.get());
  }

  void SetSemanticHistorySearchEnabled(bool enabled) {
    profile_->GetPrefs()->SetBoolean(
        local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);
  }

  BraveHistoryEmbeddingsStatus* status() {
    return BraveHistoryEmbeddingsStatus::GetForProfile(profile_.get());
  }

  base::test::ScopedFeatureList feature_list_{kHistoryEmbeddings};
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveHistoryEmbeddingsStatusTest, NoRestartWhileTheSettingIsUnchanged) {
  BuildProfileWithSetting(false);

  EXPECT_FALSE(status()->IsEnabled());
  EXPECT_FALSE(status()->NeedsRestart());

  BuildProfileWithSetting(true);

  EXPECT_TRUE(status()->IsEnabled());
  EXPECT_FALSE(status()->NeedsRestart());
}

TEST_F(BraveHistoryEmbeddingsStatusTest, RestartOnceTheSettingIsTurnedOn) {
  BuildProfileWithSetting(false);

  SetSemanticHistorySearchEnabled(true);

  // The services are still running with the setting they were built with.
  EXPECT_FALSE(status()->IsEnabled());
  EXPECT_TRUE(status()->NeedsRestart());
}

TEST_F(BraveHistoryEmbeddingsStatusTest, RestartOnceTheSettingIsTurnedOff) {
  BuildProfileWithSetting(true);

  SetSemanticHistorySearchEnabled(false);

  EXPECT_TRUE(status()->IsEnabled());
  EXPECT_TRUE(status()->NeedsRestart());
}

TEST_F(BraveHistoryEmbeddingsStatusTest, RestartClearsWhenTheSettingReverts) {
  BuildProfileWithSetting(false);
  SetSemanticHistorySearchEnabled(true);
  ASSERT_TRUE(status()->NeedsRestart());

  SetSemanticHistorySearchEnabled(false);

  EXPECT_FALSE(status()->NeedsRestart());
}

}  // namespace history_embeddings

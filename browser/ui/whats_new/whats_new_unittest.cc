/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "brave/browser/ui/whats_new/pref_names.h"
#include "brave/browser/ui/whats_new/whats_new_util.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "chrome/browser/profiles/chrome_version_service.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace whats_new {

namespace {

constexpr char kWhatsNewTrial[] = "WhatsNewStudy";

}  // namespace

class BraveWhatsNewTest : public testing::Test {
 public:
  BraveWhatsNewTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveWhatsNewTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    PrepareValidFieldTrialParams();
    ASSERT_TRUE(testing_profile_manager_.SetUp());
    profile_ = testing_profile_manager_.CreateTestingProfile("testing_profile");
    testing_profile_manager_.SetProfileAsLastUsed(profile_);

    // Make updated user.
    ChromeVersionService::SetVersion(profile_->GetPrefs(), "112.1.50.4");
  }

  void PrepareValidFieldTrialParams() {
    std::map<std::string, std::string> params;
    params[GetTargetMajorVersionParamName()] = "1.51";
    ASSERT_TRUE(
        base::AssociateFieldTrialParams(kWhatsNewTrial, "Enabled", params));
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingProfileManager testing_profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
};

TEST_F(BraveWhatsNewTest, SupportedLangTest) {
  // Prepare all other factors to show brave whats-new except lang.
  // Set current version with field trial's target major version(1.51)
  SetCurrentVersionForTesting(1.51);
  base::FieldTrialList::CreateFieldTrial(kWhatsNewTrial, "Enabled");

  // Italy is not supported yet.
  {
    const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("it_IT");
    EXPECT_FALSE(ShouldShowBraveWhatsNewForState(&local_state_));
  }

  // South korea is supported.
  {
    const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("ko_KR");
    EXPECT_TRUE(ShouldShowBraveWhatsNewForState(&local_state_));
  }
}

// Test when field trial is not available.
// base::FieldTrialList::CreateFieldTrial(kWhatsNewTrial, "Enabled") is not
// called.
TEST_F(BraveWhatsNewTest, FieldTrialNotAvailableTest) {
  // Set supported lang.
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("en_US");

  // Set current version with field trial's target major version(1.51)
  SetCurrentVersionForTesting(1.51);

  EXPECT_FALSE(ShouldShowBraveWhatsNewForState(&local_state_));
}

// Test when profile created version and current version is same.
// We treat these users as users who have never experienced updates.
// For these users, we don't launch whats-new.
TEST_F(BraveWhatsNewTest, NotUpdatedUserTest) {
  // Make not updated user. Profile created version is 1.51 and
  // current version is set as 1.51 below.
  ChromeVersionService::SetVersion(profile_->GetPrefs(), "112.1.51.4");

  // Set supported lang.
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("en_US");

  // Set current version with field trial's target major version(1.51)
  SetCurrentVersionForTesting(1.51);
  base::FieldTrialList::CreateFieldTrial(kWhatsNewTrial, "Enabled");

  EXPECT_EQ(0, local_state_.GetDouble(prefs::kWhatsNewLastVersion));
  EXPECT_FALSE(ShouldShowBraveWhatsNewForState(&local_state_));
  EXPECT_EQ(0, local_state_.GetDouble(prefs::kWhatsNewLastVersion));
}

// Test when current version and target version is matched.
TEST_F(BraveWhatsNewTest, MatchedCurrentVersionTest) {
  // Set supported lang.
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("en_US");

  // Set current version with field trial's target major version(1.51)
  SetCurrentVersionForTesting(1.51);
  base::FieldTrialList::CreateFieldTrial(kWhatsNewTrial, "Enabled");

  EXPECT_NE(1.51, local_state_.GetDouble(prefs::kWhatsNewLastVersion));
  EXPECT_TRUE(ShouldShowBraveWhatsNewForState(&local_state_));
  EXPECT_EQ(1.51, local_state_.GetDouble(prefs::kWhatsNewLastVersion));
}

// Test when current version and target version is not matched.
TEST_F(BraveWhatsNewTest, NotMatchedCurrentVersionTest) {
  // Set supported lang.
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("en_US");

  // Set different version as current version. field trial's target major
  // version is 1.51
  SetCurrentVersionForTesting(1.52);
  base::FieldTrialList::CreateFieldTrial(kWhatsNewTrial, "Enabled");

  EXPECT_FALSE(ShouldShowBraveWhatsNewForState(&local_state_));
}

// Test whats-new is already shown by setting 1.51 to prefs in advance.
TEST_F(BraveWhatsNewTest, NotWhatsNewIsAlreadyShown) {
  // Set supported lang.
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale("en_US");

  // Set different version as current version. field trial's target major
  // version is 1.51
  SetCurrentVersionForTesting(1.51);
  base::FieldTrialList::CreateFieldTrial(kWhatsNewTrial, "Enabled");

  local_state_.SetDouble(prefs::kWhatsNewLastVersion, 1.51);
  EXPECT_FALSE(ShouldShowBraveWhatsNewForState(&local_state_));
}

}  // namespace whats_new

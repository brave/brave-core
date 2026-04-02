/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/browser/test/fake_ads_service_delegate.h"
#include "brave/components/brave_ads/browser/test/fake_ads_tooltips_delegate.h"
#include "brave/components/brave_ads/browser/test/fake_bat_ads_service_factory.h"
#include "brave/components/brave_ads/browser/test/fake_device_id.h"
#include "brave/components/brave_ads/browser/test/fake_virtual_pref_provider_delegate.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_ads/browser/test/fake_rewards_service.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdsServiceImplTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(profile_dir_.CreateUniqueTempDir());

    RegisterProfilePrefs(prefs_.registry());
    brave_rewards::RegisterProfilePrefs(prefs_.registry());
    prefs_.registry()->RegisterBooleanPref(
        ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
    prefs_.registry()->RegisterBooleanPref(
        ntp_background_images::prefs::
            kNewTabPageShowSponsoredImagesBackgroundImage,
        false);

    RegisterLocalStatePrefs(local_state_.registry());
    local_state_.registry()->RegisterStringPref(
        variations::prefs::kVariationsCountry, "");

    auto device_id = std::make_unique<test::FakeDeviceId>();
    device_id_ = device_id.get();

    auto bat_ads_service_factory =
        std::make_unique<test::FakeBatAdsServiceFactory>();
    bat_ads_service_factory_ = bat_ads_service_factory.get();

    ads_service_ = std::make_unique<AdsServiceImpl>(
        std::make_unique<test::FakeAdsServiceDelegate>(), &prefs_,
        &local_state_,
        /*http_client=*/nullptr,
        std::make_unique<test::FakeVirtualPrefProviderDelegate>(),
        /*channel_name=*/"foo", profile_dir_.GetPath(),
        std::make_unique<test::FakeAdsTooltipsDelegate>(), std::move(device_id),
        std::move(bat_ads_service_factory),
        /*resource_component=*/nullptr,
        /*history_service=*/nullptr,
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
        &rewards_service_,
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)
        /*host_content_settings_map=*/nullptr);
  }

  void TearDown() override {
    // Null before reset to avoid `raw_ptr` dangling detection.
    device_id_ = nullptr;
    bat_ads_service_factory_ = nullptr;
    ads_service_->Shutdown();
    ads_service_.reset();
  }

 protected:
  void Startup() {
    // Resolves the pending device-ID request, installing pref change registrars
    // and triggering the first `MaybeStartBatAdsService` evaluation.
    device_id_->Complete();
  }

  void Shutdown() { ads_service_->Shutdown(); }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir profile_dir_;

  TestingPrefServiceSimple prefs_;
  TestingPrefServiceSimple local_state_;

  raw_ptr<test::FakeDeviceId> device_id_;  // not owned

  raw_ptr<test::FakeBatAdsServiceFactory>
      bat_ads_service_factory_;  // not owned

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  test::FakeRewardsService rewards_service_;
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

  std::unique_ptr<AdsServiceImpl> ads_service_;
};

TEST_F(BraveAdsAdsServiceImplTest, ServiceStartsWhenOptedInToSearchResultAds) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();

  // Act
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_EQ(1U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceStartsWhenOptedInToNewTabPageAds) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();

  // Act
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  ASSERT_EQ(0U, bat_ads_service_factory_->launch_count());
  prefs_.SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    true);

  // Assert
  EXPECT_EQ(1U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceInitializesAfterStarting) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();

  // Act
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_TRUE(base::test::RunUntil(
      [&] { return bat_ads_service_factory_->initialize_count() == 1U; }));
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceStopsWhenInitializationFails) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();
  bat_ads_service_factory_->set_simulate_initialization_failure();

  // Act
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return bat_ads_service_factory_->shutdown_count() == 1U; }));

  // Assert
  EXPECT_EQ(1U, bat_ads_service_factory_->launch_count());
  EXPECT_EQ(0U, bat_ads_service_factory_->initialize_count());
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceDoesNotStartWhenAlreadyRunning) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());

  // Act
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  prefs_.SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    true);

  // Assert
  EXPECT_EQ(1U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest,
       ServiceDoesNotStartWhenShuttingDownBeforeStartupCompletes) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);

  // Act
  Shutdown();
  Startup();

  // Assert
  EXPECT_EQ(0U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceDoesNotStartAfterProfileShutdown) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();
  ASSERT_EQ(0U, bat_ads_service_factory_->launch_count());

  // Act
  Shutdown();
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_EQ(0U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceStopsWhenOptedOutOfAllAds) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  prefs_.SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    true);
  Startup();
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());

  // Act
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());
  prefs_.SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    false);

  // Assert
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);
  EXPECT_EQ(2U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest,
       ServiceStartsAgainAfterOptingOutThenBackInToSearchResultAds) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());

  // Act
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_EQ(2U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest,
       ServiceStartsAgainAfterOptingOutThenBackInToNewTabPageAds) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  prefs_.SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    true);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());

  // Act
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
  ASSERT_EQ(1U, bat_ads_service_factory_->launch_count());
  prefs_.SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);

  // Assert
  EXPECT_EQ(2U, bat_ads_service_factory_->launch_count());
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
// Search result ads are opted out so the service does not start during
// `Startup`, keeping each test's trigger isolated.
TEST_F(BraveAdsAdsServiceImplTest, ServiceStartsWhenOptedInToNotificationAds) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  Startup();
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  ASSERT_EQ(0U, bat_ads_service_factory_->launch_count());

  // Act
  prefs_.SetBoolean(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_EQ(1U, bat_ads_service_factory_->launch_count());
}

TEST_F(BraveAdsAdsServiceImplTest, ServiceStartsWhenUserHasJoinedBraveRewards) {
  // Arrange
  prefs_.SetBoolean(prefs::kOptedInToSearchResultAds, false);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  // Act
  Startup();

  // Assert
  EXPECT_EQ(1U, bat_ads_service_factory_->launch_count());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

}  // namespace brave_ads

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/wallpapers/ntp_wallpaper_provider_selector.h"

#include <memory>
#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/view_counter_model.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_brave_background_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_gradient_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_solid_color_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/ntp_use_your_own_wallpaper_provider.h"
#include "brave/components/ntp_background_images/browser/wallpapers/test/fake_ntp_custom_background_delegate.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class NTPWallpaperProviderSelectorTest : public testing::Test {
 public:
  NTPWallpaperProviderSelectorTest() = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    background_images_service_ =
        std::make_unique<FakeNTPBackgroundImagesService>(
            /*variations_service=*/nullptr,
            /*component_updater_service=*/nullptr, &prefs_);
    view_counter_model_ = std::make_unique<ViewCounterModel>(&prefs_);

    use_your_own_wallpaper_provider_ =
        std::make_unique<NTPUseYourOwnWallpaperProvider>(
            custom_background_delegate_);
    gradient_wallpaper_provider_ =
        std::make_unique<NTPGradientWallpaperProvider>(
            custom_background_delegate_);
    solid_color_wallpaper_provider_ =
        std::make_unique<NTPSolidColorWallpaperProvider>(
            custom_background_delegate_);
    brave_background_wallpaper_provider_ =
        std::make_unique<NTPBraveBackgroundWallpaperProvider>(
            custom_background_delegate_, *background_images_service_,
            *view_counter_model_);

    selector_ = std::make_unique<NTPWallpaperProviderSelector>(
        *use_your_own_wallpaper_provider_, *gradient_wallpaper_provider_,
        *solid_color_wallpaper_provider_,
        *brave_background_wallpaper_provider_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  test::FakeNTPCustomBackgroundDelegate custom_background_delegate_;
  std::unique_ptr<FakeNTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<ViewCounterModel> view_counter_model_;
  std::unique_ptr<NTPUseYourOwnWallpaperProvider>
      use_your_own_wallpaper_provider_;
  std::unique_ptr<NTPGradientWallpaperProvider> gradient_wallpaper_provider_;
  std::unique_ptr<NTPSolidColorWallpaperProvider>
      solid_color_wallpaper_provider_;
  std::unique_ptr<NTPBraveBackgroundWallpaperProvider>
      brave_background_wallpaper_provider_;
  std::unique_ptr<NTPWallpaperProviderSelector> selector_;
};

TEST_F(NTPWallpaperProviderSelectorTest, ReturnsBraveBackgroundByDefault) {
  EXPECT_EQ(&selector_->GetWallpaperProvider(),
            static_cast<NTPWallpaperProvider*>(
                brave_background_wallpaper_provider_.get()));
}

TEST_F(NTPWallpaperProviderSelectorTest, ReturnsUseYourOwnWhenSet) {
  custom_background_delegate_.set_is_custom_image_background_enabled(true);

  EXPECT_EQ(&selector_->GetWallpaperProvider(),
            static_cast<NTPWallpaperProvider*>(
                use_your_own_wallpaper_provider_.get()));
}

TEST_F(NTPWallpaperProviderSelectorTest, ReturnsGradientWhenSet) {
  custom_background_delegate_.set_is_color_background_enabled(true);
  custom_background_delegate_.set_color(
      "linear-gradient(0deg, #ff0000, #0000ff)");

  EXPECT_EQ(
      &selector_->GetWallpaperProvider(),
      static_cast<NTPWallpaperProvider*>(gradient_wallpaper_provider_.get()));
}

TEST_F(NTPWallpaperProviderSelectorTest, ReturnsSolidColorWhenSet) {
  custom_background_delegate_.set_is_color_background_enabled(true);
  custom_background_delegate_.set_color("#ff0000");

  EXPECT_EQ(&selector_->GetWallpaperProvider(),
            static_cast<NTPWallpaperProvider*>(
                solid_color_wallpaper_provider_.get()));
}

TEST_F(NTPWallpaperProviderSelectorTest,
       ReturnsUseYourOwnOverGradientWhenBothEligible) {
  custom_background_delegate_.set_is_custom_image_background_enabled(true);
  custom_background_delegate_.set_is_color_background_enabled(true);
  custom_background_delegate_.set_color(
      "linear-gradient(0deg, #ff0000, #0000ff)");

  EXPECT_EQ(&selector_->GetWallpaperProvider(),
            static_cast<NTPWallpaperProvider*>(
                use_your_own_wallpaper_provider_.get()));
}

TEST_F(NTPWallpaperProviderSelectorTest,
       ReturnsUseYourOwnOverSolidColorWhenBothEligible) {
  custom_background_delegate_.set_is_custom_image_background_enabled(true);
  custom_background_delegate_.set_is_color_background_enabled(true);
  custom_background_delegate_.set_color("#ff0000");

  EXPECT_EQ(&selector_->GetWallpaperProvider(),
            static_cast<NTPWallpaperProvider*>(
                use_your_own_wallpaper_provider_.get()));
}

}  // namespace ntp_background_images

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_NTP_SPONSORED_SITE_IMAGE_SOURCE_TEST_BASE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_NTP_SPONSORED_SITE_IMAGE_SOURCE_TEST_BASE_H_

#include <memory>

#include "base/test/scoped_command_line.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_site_image_source.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

class NTPBackgroundImagesService;

namespace test {

class NTPSponsoredSiteImageSourceTestBase : public testing::Test {
 public:
  NTPSponsoredSiteImageSourceTestBase(
      const NTPSponsoredSiteImageSourceTestBase&) = delete;
  NTPSponsoredSiteImageSourceTestBase& operator=(
      const NTPSponsoredSiteImageSourceTestBase&) = delete;

  ~NTPSponsoredSiteImageSourceTestBase() override;

  // testing::Test:
  void SetUp() override;

 protected:
  NTPSponsoredSiteImageSourceTestBase();

  void SimulateOnSponsoredSitesDataDidUpdate(
      const base::FilePath& component_path);

  NTPSponsoredSiteImageSource* url_data_source() const {
    return url_data_source_.get();
  }

  content::BrowserTaskEnvironment task_environment_;

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPSponsoredSiteImageSource> url_data_source_;

 private:
  base::test::ScopedCommandLine scoped_command_line_;
};

}  // namespace test

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_TEST_NTP_SPONSORED_SITE_IMAGE_SOURCE_TEST_BASE_H_

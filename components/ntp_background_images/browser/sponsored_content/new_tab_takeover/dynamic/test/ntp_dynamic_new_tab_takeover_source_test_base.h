/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_DYNAMIC_TEST_NTP_DYNAMIC_NEW_TAB_TAKEOVER_SOURCE_TEST_BASE_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_DYNAMIC_TEST_NTP_DYNAMIC_NEW_TAB_TAKEOVER_SOURCE_TEST_BASE_H_

#include <memory>

#include "base/test/scoped_command_line.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/dynamic/ntp_dynamic_new_tab_takeover_source.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

class NTPBackgroundImagesService;

namespace test {

class NTPDynamicNewTabTakeoverSourceTestBase : public testing::Test {
 public:
  NTPDynamicNewTabTakeoverSourceTestBase(
      const NTPDynamicNewTabTakeoverSourceTestBase&) = delete;
  NTPDynamicNewTabTakeoverSourceTestBase& operator=(
      const NTPDynamicNewTabTakeoverSourceTestBase&) = delete;

  ~NTPDynamicNewTabTakeoverSourceTestBase() override;

  // testing::Test:
  void SetUp() override;

 protected:
  NTPDynamicNewTabTakeoverSourceTestBase();

  void SimulateDeprecatedOnSponsoredContentDidUpdate(
      const base::FilePath& component_path);

  NTPDynamicNewTabTakeoverSource* url_data_source() const {
    return url_data_source_.get();
  }

  content::BrowserTaskEnvironment task_environment_;

  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<NTPBackgroundImagesService> background_images_service_;
  std::unique_ptr<NTPDynamicNewTabTakeoverSource> url_data_source_;

 private:
  base::test::ScopedCommandLine scoped_command_line_;
};

}  // namespace test

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_SPONSORED_CONTENT_NEW_TAB_TAKEOVER_DYNAMIC_TEST_NTP_DYNAMIC_NEW_TAB_TAKEOVER_SOURCE_TEST_BASE_H_

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_host_content_settings_map.h"

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/tor_unittest_profile_manager.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveHostContentSettingsMapTest: public testing::Test {
 public:
  BraveHostContentSettingsMapTest()
    : local_state_(TestingBrowserProcess::GetGlobal()),
      thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }

  void SetUp() override {
    // Create a new temporary directory, and store the path
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new TorUnittestProfileManager(temp_dir_.GetPath()));

    url_ = GURL("http://testing.com/");
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
  }


  const GURL& url() {
    return url_;
  }

 protected:
  // The path to temporary directory used to contain the test operations.
  base::ScopedTempDir temp_dir_;
  ScopedTestingLocalState local_state_;

 private:
  GURL url_;
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(BraveHostContentSettingsMapTest, AskGeolocationNotInTorProfile) {
  TestingProfile profile;
  HostContentSettingsMap* host_content_settings_map =
    HostContentSettingsMapFactory::GetForProfile(&profile);
  EXPECT_EQ(CONTENT_SETTING_ASK,
            host_content_settings_map->GetContentSetting(
              url(),
              url(),
              CONTENT_SETTINGS_TYPE_GEOLOCATION,
              std::string()));
}

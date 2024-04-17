/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"

namespace {
constexpr char kTestProfileName[] = "TestProfile";
}  // namespace

class TorProfileManagerUnitTest : public testing::Test {
 public:
  TorProfileManagerUnitTest() = default;
  ~TorProfileManagerUnitTest() override = default;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_ = std::make_unique<TestingProfileManager>(browser_process);
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);
  }

  void TearDown() override {
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  Profile* profile() { return profile_; }

  Profile* GetProfileForDevToolsWindow(content::WebContents* web_contents) {
    return DevToolsWindow::GetProfileForDevToolsWindow(web_contents);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  raw_ptr<Profile> profile_ = nullptr;

  TorProfileManagerUnitTest(const TorProfileManagerUnitTest&) = delete;
  TorProfileManagerUnitTest& operator=(const TorProfileManagerUnitTest&) =
      delete;
};

TEST_F(TorProfileManagerUnitTest, InitTorProfileUserPrefs) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile());
  ASSERT_EQ(tor_profile->GetOriginalProfile(), profile());
  ASSERT_TRUE(tor_profile->IsTor());

  // Check WebRTC IP handling policy.
  EXPECT_EQ(tor_profile->GetPrefs()->GetString(prefs::kWebRTCIPHandlingPolicy),
            blink::kWebRTCIPHandlingDisableNonProxiedUdp);

  // Check SafeBrowsing status
  EXPECT_FALSE(
      tor_profile->GetPrefs()->GetBoolean(prefs::kSafeBrowsingEnabled));

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  EXPECT_FALSE(tor_profile->GetPrefs()->GetBoolean(kWebTorrentEnabled));
#endif

  // Check translate.enabled for translate bubble.
  EXPECT_FALSE(tor_profile->GetPrefs()->GetBoolean(
      translate::prefs::kOfferTranslateEnabled));
}

TEST_F(TorProfileManagerUnitTest, ProfileForDevToolsWindow) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile());
  std::unique_ptr<content::WebContents> tor_web_contents =
      content::WebContentsTester::CreateTestWebContents(tor_profile, nullptr);
  EXPECT_EQ(tor_profile, GetProfileForDevToolsWindow(tor_web_contents.get()));
}

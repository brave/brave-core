/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_drm_tab_helper.h"
#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "chrome/browser/download/download_permission_request.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/permissions/permission_prompt.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permission_request_queue.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class WidevinePermissionAndroidTest : public ChromeRenderViewHostTestHarness {
 public:
  WidevinePermissionAndroidTest()
      : ChromeRenderViewHostTestHarness(
            content::BrowserTaskEnvironment::IO_MAINLOOP) {}

  ~WidevinePermissionAndroidTest() override = default;

  permissions::PermissionRequestManager* permission_request_manager() {
    return permission_request_manager_;
  }

  permissions::PermissionRequestQueue* GetPendingRequestQueue() {
    return &(permission_request_manager_->pending_permission_requests_);
  }

  void SanityCheck() {
    EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(kAskEnableWidvine));
    EXPECT_FALSE(local_state()->GetBoolean(kWidevineEnabled));
  }

  void SimulateNavigation() {
    content::MockNavigationHandle test_handle(web_contents());
    brave_drm_tab_helper()->DidStartNavigation(&test_handle);
  }

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    // Create a test profile.
    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("Profile 1");

    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_));

    BraveDrmTabHelper::CreateForWebContents(web_contents_.get());
    tab_helper_ = BraveDrmTabHelper::FromWebContents(web_contents_.get());
    permissions::PermissionRequestManager::CreateForWebContents(
        web_contents_.get());
    permission_request_manager_ =
        permissions::PermissionRequestManager::FromWebContents(
            web_contents_.get());
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    web_contents_.reset();
    profile_manager_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  TestingPrefServiceSimple* local_state() {
    return profile_manager_->local_state()->Get();
  }
  Profile* profile() { return profile_; }
  content::WebContents* web_contents() const { return web_contents_.get(); }
  BraveDrmTabHelper* brave_drm_tab_helper() const { return tab_helper_; }

 private:
  std::unique_ptr<TestingProfileManager> profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  raw_ptr<BraveDrmTabHelper> tab_helper_ = nullptr;
  raw_ptr<permissions::PermissionRequestManager> permission_request_manager_ =
      nullptr;
};

TEST_F(WidevinePermissionAndroidTest, BraveDrmTabHelperTest) {
  SanityCheck();

  permissions::PermissionRequestManager* manager = permission_request_manager();
  EXPECT_FALSE(brave_drm_tab_helper()->ShouldShowWidevineOptIn());

  brave_drm_tab_helper()->OnWidevineKeySystemAccessRequest();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(brave_drm_tab_helper()->ShouldShowWidevineOptIn());

  EXPECT_TRUE(manager->has_pending_requests());
  EXPECT_EQ(GetPendingRequestQueue()->size(), size_t(1));

  // After navigation
  SimulateNavigation();
  EXPECT_FALSE(brave_drm_tab_helper()->ShouldShowWidevineOptIn());
  brave_drm_tab_helper()->OnWidevineKeySystemAccessRequest();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(brave_drm_tab_helper()->ShouldShowWidevineOptIn());

  // Dont ask again
  profile()->GetPrefs()->SetBoolean(kAskEnableWidvine, false);
  EXPECT_FALSE(brave_drm_tab_helper()->ShouldShowWidevineOptIn());
}

TEST_F(WidevinePermissionAndroidTest, WidevineUtilsTest) {
  SanityCheck();

  EnableWidevineCdm();
  EXPECT_TRUE(local_state()->GetBoolean(kWidevineEnabled));
  EXPECT_TRUE(IsWidevineEnabled());

  DisableWidevineCdm();
  EXPECT_FALSE(local_state()->GetBoolean(kWidevineEnabled));
  EXPECT_FALSE(IsWidevineEnabled());

  SetWidevineEnabled(true);
  EXPECT_TRUE(local_state()->GetBoolean(kWidevineEnabled));
  EXPECT_TRUE(IsWidevineEnabled());
}

TEST_F(WidevinePermissionAndroidTest, WidevinePermissionRequestTest) {
  SanityCheck();

  permissions::PermissionRequestManager* manager = permission_request_manager();

  // Accept
  brave_drm_tab_helper()->OnWidevineKeySystemAccessRequest();
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(manager->has_pending_requests() &&
              GetPendingRequestQueue()->size() == 1);
  GetPendingRequestQueue()->Pop()->PermissionGranted(false /* is_one_time */);
  EXPECT_TRUE(local_state()->GetBoolean(kWidevineEnabled));

  // Deny
  local_state()->SetBoolean(kWidevineEnabled, false);
  SimulateNavigation();
  brave_drm_tab_helper()->OnWidevineKeySystemAccessRequest();
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(manager->has_pending_requests() &&
              GetPendingRequestQueue()->size() == 1);
  GetPendingRequestQueue()->Pop()->PermissionDenied();
  EXPECT_FALSE(local_state()->GetBoolean(kWidevineEnabled));

  // Cancel
  SimulateNavigation();
  brave_drm_tab_helper()->OnWidevineKeySystemAccessRequest();
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(manager->has_pending_requests() &&
              GetPendingRequestQueue()->size() == 1);
  GetPendingRequestQueue()->Pop()->Cancelled();
  EXPECT_FALSE(local_state()->GetBoolean(kWidevineEnabled));
}

TEST_F(WidevinePermissionAndroidTest, PermissionWidevineUtilsTest) {
  SanityCheck();

  profile()->GetPrefs()->SetBoolean(kAskEnableWidvine, false);
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(kAskEnableWidvine));
  profile()->GetPrefs()->SetBoolean(kAskEnableWidvine, true);
  EXPECT_TRUE(profile()->GetPrefs()->GetBoolean(kAskEnableWidvine));

  std::vector<permissions::PermissionRequest*> requests;
  requests.push_back(new WidevinePermissionRequest(web_contents(), false));
  EXPECT_TRUE(HasWidevinePermissionRequest(requests));

  requests.push_back(new DownloadPermissionRequest(
      nullptr, url::Origin::Create(GURL("https://example.com"))));
  EXPECT_FALSE(HasWidevinePermissionRequest(requests));

  requests.erase(requests.begin());
  EXPECT_FALSE(HasWidevinePermissionRequest(requests));
}

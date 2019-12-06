/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autoplay/autoplay_permission_context.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_request_id.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class AutoplayPermissionContextTest : public AutoplayPermissionContext {
 public:
  explicit AutoplayPermissionContextTest(Profile* profile)
      : AutoplayPermissionContext(profile),
        no_tab_reloaded_(false) {}

  ~AutoplayPermissionContextTest() override {}

  bool no_tab_reloaded() { return no_tab_reloaded_; }

 protected:
  void NotifyPermissionSet(const PermissionRequestID& id,
                           const GURL& requesting_origin,
                           const GURL& embedder_origin,
                           BrowserPermissionCallback callback,
                           bool persist,
                           ContentSetting content_setting) override {
  if (!(persist && content_setting == CONTENT_SETTING_ALLOW))
    no_tab_reloaded_ = true;
  }

 private:
  bool no_tab_reloaded_;
};

}  // anonymous namespace

class AutoplayPermissionContextTests
    : public content::RenderViewHostTestHarness {
 protected:
  AutoplayPermissionContextTests() = default;

  TestingProfile* profile() {
    return static_cast<TestingProfile*>(browser_context());
  }

 private:
  // content::RenderViewHostTestHarness:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    PermissionRequestManager::CreateForWebContents(web_contents());
  }

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    return builder.Build();
  }

  DISALLOW_COPY_AND_ASSIGN(AutoplayPermissionContextTests);
};

// Autoplay permission status should still be default(block) even for
// insecure origin
TEST_F(AutoplayPermissionContextTests, TestInsecureQueryingUrl) {
  AutoplayPermissionContextTest permission_context(profile());
  GURL insecure_url("http://www.example.com");
  GURL secure_url("https://www.example.com");

  // Check that there is no saved content settings.
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            HostContentSettingsMapFactory::GetForProfile(profile())
                ->GetContentSetting(
                    insecure_url.GetOrigin(), insecure_url.GetOrigin(),
                    ContentSettingsType::AUTOPLAY, std::string()));
  EXPECT_EQ(
      CONTENT_SETTING_BLOCK,
      HostContentSettingsMapFactory::GetForProfile(profile())
          ->GetContentSetting(secure_url.GetOrigin(), insecure_url.GetOrigin(),
                              ContentSettingsType::AUTOPLAY, std::string()));
  EXPECT_EQ(
      CONTENT_SETTING_BLOCK,
      HostContentSettingsMapFactory::GetForProfile(profile())
          ->GetContentSetting(insecure_url.GetOrigin(), secure_url.GetOrigin(),
                              ContentSettingsType::AUTOPLAY, std::string()));

  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            permission_context
                .GetPermissionStatus(nullptr /* render_frame_host */,
                                     insecure_url, insecure_url)
                .content_setting);

  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            permission_context
                .GetPermissionStatus(nullptr /* render_frame_host */,
                                     insecure_url, secure_url)
                .content_setting);
}

// There is no way to generate a request that is automatically accepted in
// unittest by RequestPermission, so we test reverse cases here
TEST_F(AutoplayPermissionContextTests, TestNonAutoRefresh) {
  AutoplayPermissionContextTest permission_context(profile());
  GURL url("https://www.example.com");
  content::WebContentsTester::For(web_contents())->NavigateAndCommit(url);

  const PermissionRequestID id(
      web_contents()->GetMainFrame()->GetProcess()->GetID(),
      web_contents()->GetMainFrame()->GetRoutingID(), -1);

  // non persist allow
  HostContentSettingsMapFactory::GetForProfile(profile())
      ->SetContentSettingDefaultScope(url.GetOrigin(), url.GetOrigin(),
                                      ContentSettingsType::AUTOPLAY,
                                      std::string(), CONTENT_SETTING_ALLOW);
  permission_context.RequestPermission(
      web_contents(), id, url, true, base::DoNothing());
  EXPECT_TRUE(permission_context.no_tab_reloaded());

  // non persist block
  HostContentSettingsMapFactory::GetForProfile(profile())
      ->SetContentSettingDefaultScope(url.GetOrigin(), url.GetOrigin(),
                                      ContentSettingsType::AUTOPLAY,
                                      std::string(), CONTENT_SETTING_BLOCK);
  permission_context.RequestPermission(
      web_contents(), id, url, true, base::DoNothing());
  EXPECT_TRUE(permission_context.no_tab_reloaded());

  // no ask case because CONTENT_SETTING_ASK will cause
  // DCHECK(is_finished_) failed in `PermissionRequestImpl`. Every
  // *permission_context_unittest.cc test CONTENT_SETTING_BLOCK case now if you
  // change them to test CONTENT_SETTING_ASK, you will see the same crash stack.
}

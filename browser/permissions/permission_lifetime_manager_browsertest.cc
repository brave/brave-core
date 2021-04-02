/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_manager.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_file_value_serializer.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "base/test/test_mock_time_task_runner.h"
#include "brave/browser/permissions/mock_permission_lifetime_prompt_factory.h"
#include "brave/browser/permissions/permission_lifetime_manager_factory.h"
#include "brave/components/permissions/permission_lifetime_pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/features.h"
#include "components/permissions/request_type.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

using testing::_;

namespace permissions {

namespace {
const char kPreTestDataFileName[] = "pre_test_data";
}  // namespace

class PermissionLifetimeManagerBrowserTest : public InProcessBrowserTest {
 public:
  PermissionLifetimeManagerBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kPermissionLifetime);
  }

  ~PermissionLifetimeManagerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    PermissionRequestManager* manager = GetPermissionRequestManager();
    prompt_factory_.reset(new MockPermissionLifetimePromptFactory(manager));

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDownOnMainThread() override { prompt_factory_.reset(); }

  PermissionRequestManager* GetPermissionRequestManager() {
    return PermissionRequestManager::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(
        active_web_contents()->GetBrowserContext());
  }

  PermissionLifetimeManager* permission_lifetime_manager() {
    return PermissionLifetimeManagerFactory::GetForProfile(
        active_web_contents()->GetBrowserContext());
  }

  const util::WallClockTimer& permission_lifetime_timer() {
    return *permission_lifetime_manager()->expiration_timer_;
  }

  content::WebContents* active_web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* GetActiveMainFrame() {
    return active_web_contents()->GetMainFrame();
  }

  void ReadPreTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath user_data_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
    JSONFileValueDeserializer deserializer(
        user_data_dir.AppendASCII(kPreTestDataFileName));
    auto value = deserializer.Deserialize(nullptr, nullptr);
    ASSERT_TRUE(value);
    pre_test_data_ = std::move(*value);
    ASSERT_TRUE(pre_test_data_.is_dict());
  }

  void WritePreTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath user_data_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
    JSONFileValueSerializer serializer(
        user_data_dir.AppendASCII(kPreTestDataFileName));
    ASSERT_TRUE(serializer.Serialize(pre_test_data_));
  }

  const base::Value* GetExpirationsPrefValue() {
    return browser()->profile()->GetPrefs()->Get(
        prefs::kPermissionLifetimeExpirations);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<MockPermissionLifetimePromptFactory> prompt_factory_;
  base::Value pre_test_data_{base::Value::Type::DICTIONARY};
};

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest, ExpirationSmoke) {
  const GURL& url = embedded_test_server()->GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), url);
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);

  base::RunLoop run_loop;
  std::unique_ptr<base::ScopedMockTimeMessageLoopTaskRunner>
      scoped_mock_time_task_runner;
  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([&](MockPermissionLifetimePrompt* prompt) {
        run_loop.Quit();
        prompt->delegate()->Requests()[0]->SetLifetime(
            base::TimeDelta::FromSeconds(30));
        scoped_mock_time_task_runner =
            std::make_unique<base::ScopedMockTimeMessageLoopTaskRunner>();
      }));
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  run_loop.Run();

  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue()->DictEmpty());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  scoped_mock_time_task_runner->task_runner()->FastForwardBy(
      base::TimeDelta::FromSeconds(20));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  scoped_mock_time_task_runner->task_runner()->FastForwardBy(
      base::TimeDelta::FromSeconds(20));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_TRUE(GetExpirationsPrefValue()->DictEmpty());
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest,
                       PRE_PermissionExpiredAfterRestart) {
  const GURL& url = embedded_test_server()->GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), url);
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);
  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([&](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(
            base::TimeDelta::FromSeconds(30));
      }));

  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  pre_test_data_.SetStringKey("url", url.spec());
  WritePreTestData();
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest,
                       PermissionExpiredAfterRestart) {
  ReadPreTestData();
  const GURL url(*pre_test_data_.FindStringKey("url"));

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue()->DictEmpty());

  base::ScopedMockTimeMessageLoopTaskRunner scoped_mock_time_task_runner;
  permission_lifetime_manager()->RestartExpirationTimerForTesting();
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue()->DictEmpty());

  scoped_mock_time_task_runner.task_runner()->FastForwardBy(
      base::TimeDelta::FromSeconds(10));
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  scoped_mock_time_task_runner.task_runner()->FastForwardBy(
      base::TimeDelta::FromSeconds(60));
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_TRUE(GetExpirationsPrefValue()->DictEmpty());
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest,
                       ExpirationRemovedAfterManualReset) {
  const GURL& url = embedded_test_server()->GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), url);
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);
  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([&](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(
            base::TimeDelta::FromSeconds(30));
      }));

  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue()->DictEmpty());

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::GEOLOCATION,
      ContentSetting::CONTENT_SETTING_DEFAULT);
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_TRUE(GetExpirationsPrefValue()->DictEmpty());
}

}  // namespace permissions
/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/permissions/permission_lifetime_manager.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_file_value_serializer.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
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
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/features.h"
#include "components/permissions/request_type.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/features.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

using testing::_;

namespace permissions {

namespace {
const int kKeepAliveInterval = 2;
const char kPreTestDataFileName[] = "pre_test_data";
}  // namespace

class PermissionLifetimeManagerBrowserTest : public InProcessBrowserTest {
 public:
  PermissionLifetimeManagerBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~PermissionLifetimeManagerBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    PermissionRequestManager* manager = GetPermissionRequestManager();
    prompt_factory_ =
        std::make_unique<MockPermissionLifetimePromptFactory>(manager);

    host_resolver()->AddRule("*", "127.0.0.1");
    https_server()->ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_server()->Start());
  }

  void TearDownOnMainThread() override { prompt_factory_.reset(); }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

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

  const base::WallClockTimer& permission_lifetime_timer() {
    return *permission_lifetime_manager()->expiration_timer_;
  }

  content::WebContents* active_web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* GetActiveMainFrame() {
    return active_web_contents()->GetPrimaryMainFrame();
  }

  void ReadPreTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath user_data_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
    JSONFileValueDeserializer deserializer(
        user_data_dir.AppendASCII(kPreTestDataFileName));
    auto value = deserializer.Deserialize(nullptr, nullptr);
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_dict());
    pre_test_data_ = std::move(value->GetDict());
  }

  void WritePreTestData() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath user_data_dir;
    base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
    JSONFileValueSerializer serializer(
        user_data_dir.AppendASCII(kPreTestDataFileName));
    ASSERT_TRUE(serializer.Serialize(pre_test_data_));
  }

  const base::Value::Dict& GetExpirationsPrefValue() {
    return browser()->profile()->GetPrefs()->GetDict(
        prefs::kPermissionLifetimeExpirations);
  }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  std::unique_ptr<MockPermissionLifetimePromptFactory> prompt_factory_;
  base::Value::Dict pre_test_data_;
};

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest, ExpirationSmoke) {
  const GURL& url = https_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);

  base::RunLoop run_loop;
  std::unique_ptr<base::ScopedMockTimeMessageLoopTaskRunner>
      scoped_mock_time_task_runner;
  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([&](MockPermissionLifetimePrompt* prompt) {
        run_loop.Quit();
        prompt->delegate()->Requests()[0]->SetLifetime(base::Seconds(30));
        scoped_mock_time_task_runner =
            std::make_unique<base::ScopedMockTimeMessageLoopTaskRunner>();
      }));
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  run_loop.Run();

  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  scoped_mock_time_task_runner->task_runner()->FastForwardBy(base::Seconds(20));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  scoped_mock_time_task_runner->task_runner()->FastForwardBy(base::Seconds(20));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest,
                       PRE_PermissionExpiredAfterRestart) {
  const GURL& url = https_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);
  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([&](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(base::Seconds(30));
      }));

  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  pre_test_data_.Set("url", url.spec());
  WritePreTestData();
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest,
                       PermissionExpiredAfterRestart) {
  ReadPreTestData();
  const GURL url(*pre_test_data_.FindString("url"));

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  base::ScopedMockTimeMessageLoopTaskRunner scoped_mock_time_task_runner;
  permission_lifetime_manager()->RestartExpirationTimerForTesting();
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  scoped_mock_time_task_runner.task_runner()->FastForwardBy(base::Seconds(10));
  EXPECT_TRUE(permission_lifetime_timer().IsRunning());
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  scoped_mock_time_task_runner.task_runner()->FastForwardBy(base::Seconds(60));
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerBrowserTest,
                       ExpirationRemovedAfterManualReset) {
  const GURL& url = https_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);
  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([&](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(base::Seconds(30));
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
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  host_content_settings_map()->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::GEOLOCATION,
      ContentSetting::CONTENT_SETTING_DEFAULT);
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
}

class PermissionLifetimeManagerWithOriginMonitorBrowserTest
    : public PermissionLifetimeManagerBrowserTest {
 public:
  PermissionLifetimeManagerWithOriginMonitorBrowserTest() {
    ephemeral_storage::EphemeralStorageTabHelper::
        SetKeepAliveTimeDelayForTesting(base::Seconds(kKeepAliveInterval));
  }
};

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerWithOriginMonitorBrowserTest,
                       DomainPermissionReset) {
  const GURL& url = https_server()->GetURL("host.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);

  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(base::TimeDelta());
      }));
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  // Navigate to another domain. It should not reset the permission.
  const GURL& other_url =
      https_server()->GetURL("other_host.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), other_url));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  // Permission Should be reset after the timeout
  base::RunLoop run_loop;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(kKeepAliveInterval));
  run_loop.Run();

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerWithOriginMonitorBrowserTest,
                       FriendlyDomainPermissionKept) {
  const GURL& url = https_server()->GetURL("example.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);

  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(base::TimeDelta());
      }));
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  // Navigate to a subdomain, permission should be kept.
  const GURL& sub_url =
      https_server()->GetURL("sub.example.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), sub_url));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  // Navigate to another domain. It should keep the permission.
  const GURL& other_url =
      https_server()->GetURL("other_host.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), other_url));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  // Permission Should be reset after the timeout
  base::RunLoop run_loop;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(kKeepAliveInterval));
  run_loop.Run();

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerWithOriginMonitorBrowserTest,
                       PublicSuffixListDomainPermissionReset) {
  const GURL& url = https_server()->GetURL("user.github.io", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);

  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(base::TimeDelta());
      }));
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  // Navigate to a subdomain, permission should be kept.
  const GURL& sub_url =
      https_server()->GetURL("sub.user.github.io", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), sub_url));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  // Navigate to another domain in PSL. It should keep the permission.
  const GURL& other_url =
      https_server()->GetURL("user2.github.io", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), other_url));
  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  // Permission Should be reset after the timeout
  base::RunLoop run_loop;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(kKeepAliveInterval));
  run_loop.Run();

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerWithOriginMonitorBrowserTest,
                       PRE_DomainPermissionResetAfterRestart) {
  const GURL& url = https_server()->GetURL("example.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  prompt_factory_->set_response_type(
      PermissionRequestManager::AutoResponseType::ACCEPT_ALL);

  EXPECT_CALL(*prompt_factory_, OnPermissionPromptCreated(_))
      .WillOnce(testing::Invoke([](MockPermissionLifetimePrompt* prompt) {
        prompt->delegate()->Requests()[0]->SetLifetime(base::TimeDelta());
      }));
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  prompt_factory_->WaitForPermissionBubble();

  EXPECT_EQ(1, prompt_factory_->show_count());
  EXPECT_FALSE(permission_lifetime_timer().IsRunning());
  EXPECT_FALSE(GetExpirationsPrefValue().empty());

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ALLOW);

  pre_test_data_.Set("url", url.spec());
  WritePreTestData();
}

IN_PROC_BROWSER_TEST_F(PermissionLifetimeManagerWithOriginMonitorBrowserTest,
                       DomainPermissionResetAfterRestart) {
  ReadPreTestData();
  const GURL url(*pre_test_data_.FindString("url"));

  EXPECT_EQ(host_content_settings_map()->GetContentSetting(
                url, url, ContentSettingsType::GEOLOCATION),
            ContentSetting::CONTENT_SETTING_ASK);
  EXPECT_TRUE(GetExpirationsPrefValue().empty());
}

}  // namespace permissions

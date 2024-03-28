/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/ui/geolocation/geolocation_accuracy_tab_helper.h"
#include "brave/browser/ui/geolocation/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "components/prefs/pref_service.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class GeolocationAccuracyBrowserTest : public InProcessBrowserTest {
 public:
  GeolocationAccuracyBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  GeolocationAccuracyBrowserTest(const GeolocationAccuracyBrowserTest&) =
      delete;
  GeolocationAccuracyBrowserTest& operator=(
      const GeolocationAccuracyBrowserTest&) = delete;
  ~GeolocationAccuracyBrowserTest() override = default;

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
    permissions::PermissionRequestManager* manager =
        permissions::PermissionRequestManager::FromWebContents(
            GetActiveWebContents());
    mock_permission_prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(manager);

    host_resolver()->AddRule("*", "127.0.0.1");
    https_server()->ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_server()->Start());
  }

  void TearDownOnMainThread() override {
    mock_permission_prompt_factory_.reset();
  }

  permissions::MockPermissionPromptFactory* prompt_factory() {
    return mock_permission_prompt_factory_.get();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }
  base::RunLoop* run_loop() const { return run_loop_.get(); }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run()) {
      return;
    }

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition]() {
                      if (condition.Run()) {
                        run_loop_->Quit();
                      }
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* GetActiveMainFrame() {
    return GetActiveWebContents()->GetPrimaryMainFrame();
  }

  void AcceptDialogForTesting() {
    web_modal::WebContentsModalDialogManager* manager =
        web_modal::WebContentsModalDialogManager::FromWebContents(
            GetActiveWebContents());
    DCHECK(manager);
    manager->CloseAllDialogs();
  }

  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  std::unique_ptr<permissions::MockPermissionPromptFactory>
      mock_permission_prompt_factory_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(GeolocationAccuracyBrowserTest, DialogLaunchTest) {
  const GURL& url = https_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  mock_permission_prompt_factory_->set_response_type(
      permissions::PermissionRequestManager::AutoResponseType::DISMISS);

  // Check bubble is shown and dialog is also launched.
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  mock_permission_prompt_factory_->WaitForPermissionBubble();

  GeolocationAccuracyTabHelper* accuracy_tab_helper =
      GeolocationAccuracyTabHelper::FromWebContents(GetActiveWebContents());
  if (!accuracy_tab_helper) {
    return;
  }

  EXPECT_TRUE(accuracy_tab_helper->is_dialog_running_);
  EXPECT_TRUE(accuracy_tab_helper->dialog_asked_in_current_navigation_);

  // Wait to dialog closing.
  AcceptDialogForTesting();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !accuracy_tab_helper->is_dialog_running_; }));
  EXPECT_TRUE(accuracy_tab_helper->dialog_asked_in_current_navigation_);

  // Navigate again and check flags are cleared.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_FALSE(accuracy_tab_helper->is_dialog_running_);
  EXPECT_FALSE(accuracy_tab_helper->dialog_asked_in_current_navigation_);

  // Disable dialog launching.
  browser()->profile()->GetPrefs()->SetBoolean(
      kShowGeolocationAccuracyHelperDialog, false);

  // Check bubble is shown again but dialog is not launched.
  content::ExecuteScriptAsync(
      GetActiveMainFrame(),
      "navigator.geolocation.getCurrentPosition(function(){});");
  mock_permission_prompt_factory_->WaitForPermissionBubble();
  EXPECT_FALSE(accuracy_tab_helper->is_dialog_running_);
}

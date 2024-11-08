/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_brave_search_throttle.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kBraveSearchHost[] = "search.brave.com";
constexpr char kLeoPath[] = "/leo";
constexpr char kOpenAIChatButtonValidPath[] = "/open_ai_chat_button_valid.html";
constexpr char kOpenAIChatButtonInvalidPath[] =
    "/open_ai_chat_button_invalid.html";

}  // namespace

// TODO(jocelyn): This should be changed to PlatformBrowserTest when we support
// Android. https://github.com/brave/brave-browser/issues/41905
class AIChatBraveSearchThrottleBrowserTest : public InProcessBrowserTest {
 public:
  AIChatBraveSearchThrottleBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(&https_server_);

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII("leo");
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_.Start());

    permissions::PermissionRequestManager* manager =
        permissions::PermissionRequestManager::FromWebContents(
            GetActiveWebContents());
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(manager);
  }

  void TearDownOnMainThread() override {
    prompt_factory_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ClickOpenAIChatButton() {
    // Modify the href to have test server port and click it.
    ASSERT_TRUE(content::ExecJs(GetActiveWebContents()->GetPrimaryMainFrame(),
                                content::JsReplace(R"(
            const link = document.getElementById('continue-with-leo')
            const url = new URL(link.href)
            url.port = $1
            link.href = url.href
            link.click())",
                                                   https_server_.port())));
  }

  bool IsLeoOpened() {
    sidebar::SidebarController* controller =
        static_cast<BraveBrowser*>(browser())->sidebar_controller();
    auto index = controller->model()->GetIndexOf(
        sidebar::SidebarItem::BuiltInItemType::kChatUI);
    return index.has_value() && controller->IsActiveIndex(index);
  }

  void CloseLeoPanel(const base::Location& location) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    sidebar::SidebarController* controller =
        static_cast<BraveBrowser*>(browser())->sidebar_controller();
    controller->DeactivateCurrentPanel();
    ASSERT_FALSE(IsLeoOpened());
  }

  void NavigateToTestPage(const base::Location& location,
                          const std::string& host,
                          const std::string& path,
                          int expected_prompt_count) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                       https_server_.GetURL(host, path)));
    EXPECT_FALSE(IsLeoOpened());
    EXPECT_EQ(expected_prompt_count, prompt_factory_->show_count());
  }

  void ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      const base::Location& location,
      int expected_prompt_count,
      bool expected_leo_opened,
      const std::string& expected_last_committed_path =
          kOpenAIChatButtonValidPath) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    content::TestNavigationObserver observer(
        GetActiveWebContents(), net::ERR_ABORTED,
        content::MessageLoopRunner::QuitMode::IMMEDIATE,
        false /* ignore_uncommitted_navigations */);
    ClickOpenAIChatButton();
    observer.Wait();

    EXPECT_EQ(IsLeoOpened(), expected_leo_opened);
    EXPECT_EQ(expected_prompt_count, prompt_factory_->show_count());
    EXPECT_EQ(observer.last_navigation_url().path_piece(), kLeoPath);
    EXPECT_EQ(GetActiveWebContents()->GetLastCommittedURL().path_piece(),
              expected_last_committed_path);
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(AIChatBraveSearchThrottleBrowserTest,
                       OpenAIChat_AskAndAccept) {
  int cur_prompt_count = 0;
  prompt_factory_->set_response_type(
      permissions::PermissionRequestManager::ACCEPT_ALL);
  NavigateToTestPage(FROM_HERE, kBraveSearchHost, kOpenAIChatButtonValidPath,
                     cur_prompt_count);
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, ++cur_prompt_count,
      /*expected_leo_opened=*/true);

  CloseLeoPanel(FROM_HERE);
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, cur_prompt_count,
      /*expected_leo_opened=*/true);
}

IN_PROC_BROWSER_TEST_F(AIChatBraveSearchThrottleBrowserTest,
                       OpenAIChat_AskAndDeny) {
  int cur_prompt_count = 0;
  prompt_factory_->set_response_type(
      permissions::PermissionRequestManager::DENY_ALL);
  NavigateToTestPage(FROM_HERE, kBraveSearchHost, kOpenAIChatButtonValidPath,
                     cur_prompt_count);
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, ++cur_prompt_count, /*expected_leo_opened=*/false);

  // Clicking a button again to test no new permission prompt should be shown
  // when the permission setting is denied.
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, cur_prompt_count,
      /*expected_leo_opened=*/false);
}

IN_PROC_BROWSER_TEST_F(AIChatBraveSearchThrottleBrowserTest,
                       OpenAIChat_AskAndDismiss) {
  int cur_prompt_count = 0;
  prompt_factory_->set_response_type(
      permissions::PermissionRequestManager::DISMISS);
  NavigateToTestPage(FROM_HERE, kBraveSearchHost, kOpenAIChatButtonValidPath,
                     cur_prompt_count);
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, ++cur_prompt_count, /*expected_leo_opened=*/false);

  // Click a button again after dismissing the permission, permission prompt
  // should be shown again.
  prompt_factory_->set_response_type(
      permissions::PermissionRequestManager::ACCEPT_ALL);
  NavigateToTestPage(FROM_HERE, kBraveSearchHost, kOpenAIChatButtonValidPath,
                     cur_prompt_count);
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, ++cur_prompt_count,
      /*expected_leo_opened=*/true);
}

IN_PROC_BROWSER_TEST_F(AIChatBraveSearchThrottleBrowserTest,
                       OpenAIChat_MismatchedNonce) {
  int cur_prompt_count = 0;
  NavigateToTestPage(FROM_HERE, kBraveSearchHost, kOpenAIChatButtonInvalidPath,
                     cur_prompt_count);
  // No permission prompt should be shown.
  ClickOpenAIChatAndCheckLeoOpenedAndNavigationCancelled(
      FROM_HERE, cur_prompt_count, /*expected_leo_opened=*/false,
      kOpenAIChatButtonInvalidPath);
}

IN_PROC_BROWSER_TEST_F(AIChatBraveSearchThrottleBrowserTest,
                       OpenAIChat_NotBraveSearchURL) {
  // The behavior should be the same as without the throttle.
  NavigateToTestPage(FROM_HERE, "brave.com", kOpenAIChatButtonValidPath, 0);
  content::TestNavigationObserver observer(GetActiveWebContents());
  ClickOpenAIChatButton();
  observer.Wait();

  EXPECT_FALSE(IsLeoOpened());
  EXPECT_EQ(0, prompt_factory_->show_count());
  EXPECT_TRUE(observer.last_navigation_succeeded());
  EXPECT_EQ(observer.last_navigation_url().path_piece(), kLeoPath);
  EXPECT_EQ(GetActiveWebContents()->GetLastCommittedURL().path_piece(),
            kLeoPath);
}

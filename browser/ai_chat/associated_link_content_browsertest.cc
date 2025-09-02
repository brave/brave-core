// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/associated_link_content.h"

#include <memory>
#include <string>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AssociatedLinkContentBrowserTest : public InProcessBrowserTest {
 public:
  AssociatedLinkContentBrowserTest() = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir.AppendASCII("ai_chat"));

    https_server_.StartAcceptingConnections();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  }

  void SetUp() override {
    ASSERT_TRUE(https_server_.InitializeAndListen());
    InProcessBrowserTest::SetUp();
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

 protected:
  // Helper method to wait for content loading
  PageContent WaitForContent(AssociatedLinkContent* link_content) {
    base::RunLoop run_loop;
    PageContent result;

    link_content->GetContent(
        base::BindLambdaForTesting([&](PageContent content) {
          result = std::move(content);
          run_loop.Quit();
        }));

    run_loop.Run();
    return result;
  }

  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(AssociatedLinkContentBrowserTest,
                       SuccessfulBackgroundLoading) {
  GURL test_url = https_server_.GetURL("/basic.html");

  auto link_content = std::make_unique<AssociatedLinkContent>(
      test_url, u"Title", browser()->profile());

  PageContent content = WaitForContent(link_content.get());

  EXPECT_FALSE(content.is_video);
  EXPECT_TRUE(base::Contains(content.content, "Hello World!"));
}

IN_PROC_BROWSER_TEST_F(AssociatedLinkContentBrowserTest,
                       MultipleGetConentsDoesNotBreak) {
  GURL test_url = https_server_.GetURL("/basic.html");

  auto link_content = std::make_unique<AssociatedLinkContent>(
      test_url, u"Title", browser()->profile());

  // Repeat the load a few times to make sure nothing breaks. In the current
  // implementation we only fetch once but that may change in the future, so we
  // don't enforce it here.
  for (size_t i = 0; i < 10; i++) {
    PageContent content = WaitForContent(link_content.get());

    EXPECT_FALSE(content.is_video);
    EXPECT_TRUE(base::Contains(content.content, "Hello World!"));
  }
}

IN_PROC_BROWSER_TEST_F(AssociatedLinkContentBrowserTest,
                       HandlesNavigationErrors) {
  // Test with non-existent URL that will return 404
  GURL invalid_url = https_server_.GetURL("/non-existent-page.html");

  auto link_content = std::make_unique<AssociatedLinkContent>(
      invalid_url, u"Invalid URL Title", browser()->profile());

  // Should handle 404 gracefully
  PageContent content = WaitForContent(link_content.get());
  // Content should be empty on error
  EXPECT_TRUE(content.content.empty());
}

IN_PROC_BROWSER_TEST_F(AssociatedLinkContentBrowserTest,
                       HandlesMultipleConcurrentRequests) {
  GURL test_url = https_server_.GetURL("/basic.html");

  auto link_content = std::make_unique<AssociatedLinkContent>(
      test_url, u"Title",
      static_cast<content::BrowserContext*>(browser()->profile()));

  // Make multiple concurrent requests
  base::RunLoop run_loop;
  std::vector<PageContent> results;
  int completed_requests = 0;

  auto callback = base::BindLambdaForTesting([&](PageContent content) {
    results.push_back(std::move(content));
    completed_requests++;
    if (completed_requests == 3) {
      run_loop.Quit();
    }
  });

  // Start three concurrent requests
  link_content->GetContent(callback);
  link_content->GetContent(callback);
  link_content->GetContent(callback);

  run_loop.Run();

  // All requests should return the same content
  EXPECT_EQ(3u, results.size());
  for (const auto& result : results) {
    EXPECT_TRUE(base::Contains(result.content, "Hello World!"));
    EXPECT_EQ(results[0].content, result.content);
  }
}

}  // namespace ai_chat

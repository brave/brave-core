// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_player/brave_player_service_factory.h"
#include "brave/components/brave_player/core/browser/brave_player_service.h"
#include "brave/components/brave_player/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#endif

namespace brave_player {

class BravePlayerTabHelperBrowserTest : public PlatformBrowserTest {
 public:
  BravePlayerTabHelperBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitWithFeatures(
        {brave_player::features::kBravePlayer,
         brave_player::features::kBravePlayerRespondToAntiAdBlock},
        {});
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    content::SetBrowserClientForTesting(&test_content_browser_client_);

#if defined(IS_ANDROID)
    Profile* profile = TabModelList::models()[0]->GetProfile();
#else
    Profile* profile = browser()->profile();
#endif

    ASSERT_TRUE(profile);

    // Also called in Disabled test.
    brave_player::BravePlayerServiceFactory::GetForBrowserContext(profile)
        .SetComponentPath(test_data_dir.AppendASCII("brave_player_component"));

    https_server_.ServeFilesFromDirectory(test_data_dir);

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  bool DialogIsVisible() {
    // TODO - requires dialog implementation.
    return false;
  }

 protected:
  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList feature_list_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  BraveContentBrowserClient test_content_browser_client_;
};

IN_PROC_BROWSER_TEST_F(BravePlayerTabHelperBrowserTest, YoutubeInjection) {
  // Must use HTTPS because `youtube.com` is in Chromium's HSTS preload list
  GURL url = https_server_.GetURL("youtube.com", "/simple.html");

  std::u16string expected_title(u"success");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  // ASSERT_TRUE(DialogIsVisible());
}

IN_PROC_BROWSER_TEST_F(BravePlayerTabHelperBrowserTest, NotYoutubeNoInjection) {
  GURL url = https_server_.GetURL("not-youtube.com", "/simple.html");

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  ASSERT_TRUE(!DialogIsVisible());
}

class BravePlayerTabHelperBrowserTestDisabled
    : public BravePlayerTabHelperBrowserTest {
 public:
  BravePlayerTabHelperBrowserTestDisabled() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(brave_player::features::kBravePlayer);
  }
};

IN_PROC_BROWSER_TEST_F(BravePlayerTabHelperBrowserTest, NoYoutubeInjection) {
  // Must use HTTPS because `youtube.com` is in Chromium's HSTS preload list
  GURL url = https_server_.GetURL("youtube.com", "/simple.html");

  std::u16string expected_title(u"OK");
  content::TitleWatcher watcher(web_contents(), expected_title);
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url));
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());

  ASSERT_TRUE(!DialogIsVisible());
}

}  // namespace brave_player

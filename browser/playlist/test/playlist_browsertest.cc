/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_service.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_download_request_manager.h"
#include "brave/components/playlist/browser/playlist_service_observer.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"

class PlaylistBrowserTest : public PlatformBrowserTest {
 public:
  PlaylistBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~PlaylistBrowserTest() override = default;

  auto* https_server() { return https_server_.get(); }

  GURL GetURL(const std::string& path) { return https_server()->GetURL(path); }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

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
    run_loop_->Run();
  }

  playlist::PlaylistService* GetService() {
    return playlist::PlaylistServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  // PlatformBrowserTest:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());

    auto* service = GetService();
    service->download_request_manager_->media_detector_component_manager()
        ->SetUseLocalScriptForTesting();
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

 private:
  std::unique_ptr<base::RunLoop> run_loop_;

  base::test::ScopedFeatureList scoped_feature_list_;

  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, AddItemsToList) {
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GetURL("/playlist/site_with_video.html")));

  chrome::AddTabAt(browser(), {}, -1, true /*foreground*/);
  ASSERT_TRUE(
      content::NavigateToURL(GetActiveWebContents(), GURL(kPlaylistURL)));
  auto* playlist_web_contents = GetActiveWebContents();

  browser()->tab_strip_model()->ActivateTabAt(0);

  ASSERT_TRUE(content::ExecJs(
      playlist_web_contents,
      "document.querySelector('#download-from-active-tab-btn').click();"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    return content::EvalJs(playlist_web_contents,
                           "!!document.querySelector('.playlist-item');")
        .ExtractBool();
  }));
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_RemoveItemFromList) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_ThumbnailFailed) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_MediaDownloadFailed) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_ApiFunctions) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_CreateAndRemovePlaylist) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, RemoveAndRestoreLocalData) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, PlayWithoutLocalCache) {
  // Create an item and wait for it to be cached.
  ASSERT_TRUE(content::NavigateToURL(
      GetActiveWebContents(),
      https_server()->GetURL("test.googlevideo.com",
                             "/playlist/site_with_video.html")));

  chrome::AddTabAt(browser(), {}, -1, true /*foreground*/);
  auto* playlist_contents = GetActiveWebContents();
  ASSERT_TRUE(content::NavigateToURL(playlist_contents, GURL(kPlaylistURL)));

  browser()->tab_strip_model()->ActivateTabAt(0);

  ASSERT_TRUE(content::ExecJs(
      playlist_contents,
      "document.querySelector('#download-from-active-tab-btn').click();"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    auto result = content::EvalJs(playlist_contents,
                                  R"-js(
          const item = document.querySelector('.playlist-item');
          item && item.parentElement.parentElement
              .querySelector('.playlist-item-cached-state')
              .textContent == 'Cached';
        )-js");

    return !result.value.is_none() && result.ExtractBool();
  }));

  // Remove cache
  ASSERT_TRUE(content::ExecJs(playlist_contents,
                              R"-js(
          const item = document.querySelector('.playlist-item');
          item.parentElement.parentElement
              .querySelector('.playlist-item-cache-btn').click();
        )-js"));
  WaitUntil(base::BindLambdaForTesting([&]() {
    auto result = content::EvalJs(playlist_contents,
                                  R"-js(
          const item = document.querySelector('.playlist-item');
          item && item.parentElement.parentElement
              .querySelector('.playlist-item-cached-state')
              .textContent != 'Cached';
       )-js");

    return !result.value.is_none() && result.ExtractBool();
  }));

  // Try playing the item
  browser()->tab_strip_model()->ActivateTabAt(1);
  ASSERT_TRUE(content::ExecJs(playlist_contents,
                              R"-js(
          document.querySelector('.playlist-item-thumbnail').click();
        )-js"));
  WaitUntil(base::BindLambdaForTesting([&]() {
    return content::EvalJs(playlist_contents,
                           R"-js(
          document.querySelector('#player')
          .getAttribute('data-playing') === 'true';
        )-js")
        .ExtractBool();
  }));
}

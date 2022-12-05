/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_download_request_manager.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"

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
    if (condition.Run())
      return;

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition]() {
                      if (condition.Run())
                        run_loop_->Quit();
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
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

    auto* service = playlist::PlaylistServiceFactory::GetForBrowserContext(
        browser()->profile());
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

  ASSERT_TRUE(content::ExecJs(
      GetActiveWebContents(),
      "document.querySelector('#download-from-open-tabs-btn').click();"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    return content::EvalJs(GetActiveWebContents(),
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
  VLOG(2) << "create playlist 1";
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  // pre condition: there's an already downloaded playlist item.
  auto items = service->GetAllPlaylistItems();
  ASSERT_EQ(1UL, items.size());

  auto item = items.front();
  ASSERT_TRUE(item.media_file_cached);
  ASSERT_NE(item.media_src, item.media_file_path);
  ASSERT_NE(item.thumbnail_src, item.thumbnail_path);
  auto dir_path = service->GetPlaylistItemDirPath(item.id);
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::DirectoryExists(dir_path));
  }

  // Store the item's local file path first
  items = service->GetAllPlaylistItems();
  item = items.front();
  base::FilePath media_path;
  base::FilePath thumbnail_path;
  ASSERT_TRUE(service->GetMediaPath(item.id, &media_path));
  ASSERT_TRUE(service->GetThumbnailPath(item.id, &thumbnail_path));

  // Remove local data for the item. When we remove local data, we remove only
  // media file.
  service->DeletePlaylistLocalData(items.front().id);
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(1UL, items.size());
  item = items.front();

  auto file_exists = [](const base::FilePath& path) {
    return base::PathExists(path) && !base::DirectoryExists(path);
  };

  // Values are updated first and then the data from disk will be removed.
  EXPECT_FALSE(item.media_file_cached);
  EXPECT_EQ(item.media_src, item.media_file_path);
  EXPECT_NE(item.thumbnail_src, item.thumbnail_path);
  WaitUntil(base::BindLambdaForTesting([&]() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return !file_exists(media_path);
  }));

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::DirectoryExists(dir_path));
    EXPECT_TRUE(file_exists(thumbnail_path));
    EXPECT_FALSE(file_exists(media_path));
  }

  // Restore local data for the item.
  service->RecoverPlaylistItem(item.id);
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(1UL, items.size());

  item = items.front();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::DirectoryExists(dir_path));
  }

  WaitUntil(base::BindLambdaForTesting([&]() {
    auto items = service->GetAllPlaylistItems();
    return items.size() && items.front().media_file_cached;
  }));
  item = service->GetAllPlaylistItems().front();
  EXPECT_NE(item.media_src, item.media_file_path);

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::DirectoryExists(dir_path));
    EXPECT_TRUE(file_exists(thumbnail_path));
    EXPECT_TRUE(file_exists(media_path));
  }
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, PlayWithoutLocalCache) {
  // Create an item and wait for it to be cached.
  ASSERT_TRUE(content::NavigateToURL(
      GetActiveWebContents(),
      https_server()->GetURL("test.googlevideo.com",
                             "/playlist/site_with_video.html")));

  chrome::AddTabAt(browser(), {}, -1, true /*foreground*/);
  ASSERT_TRUE(
      content::NavigateToURL(GetActiveWebContents(), GURL(kPlaylistURL)));

  ASSERT_TRUE(content::ExecJs(
      GetActiveWebContents(),
      "document.querySelector('#download-from-open-tabs-btn').click();"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    auto result = content::EvalJs(GetActiveWebContents(),
                                  R"-js(
          const item = document.querySelector('.playlist-item');
          item && item.parentElement.parentElement
              .querySelector('.playlist-item-cached-state')
              .textContent == 'Cached';
        )-js");

    return !result.value.is_none() && result.ExtractBool();
  }));

  // Remove cache
  LOG(ERROR) << "1))))";
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents(),
                              R"-js(
          const item = document.querySelector('.playlist-item');
          item.parentElement.parentElement
              .querySelector('.playlist-item-cache-btn').click();
        )-js"));
  WaitUntil(base::BindLambdaForTesting([&]() {
    auto result = content::EvalJs(GetActiveWebContents(),
                                  R"-js(
          const item = document.querySelector('.playlist-item');
          item && item.parentElement.parentElement
              .querySelector('.playlist-item-cached-state')
              .textContent != 'Cached';
       )-js");

    return !result.value.is_none() && result.ExtractBool();
  }));

  LOG(ERROR) << "2))))";
  // Try playing the item
  ASSERT_TRUE(content::ExecJs(GetActiveWebContents(),
                              R"-js(
          document.querySelector('.playlist-item-thumbnail').click();
        )-js"));
  WaitUntil(base::BindLambdaForTesting([&]() {
    return content::EvalJs(GetActiveWebContents(),
                           R"-js(
          document.querySelector('#player')
          .getAttribute('data-playing') === 'true';
        )-js")
        .ExtractBool();
  }));
}

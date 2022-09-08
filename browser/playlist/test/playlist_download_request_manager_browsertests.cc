/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_download_request_manager.h"

#include "base/ranges/algorithm.h"
#include "brave/components/playlist/media_detector_component_manager.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

// Usage of this matcher:
//   std::vector<PlaylistItemInfo> actual = { ... };
//   std::vector<PlaylistItemInfo> expected = { ... };
//   EXPECT_THAT(actual, IsSamePlaylistItems(expected));
MATCHER_P(IsSamePlaylistItems, expected, "") {
  auto equal = [](const auto& a, const auto& b) {
    // id is not compared because it is generated for actual items.
    return a.media_file_path == b.media_file_path && a.title == b.title &&
           a.thumbnail_path == b.thumbnail_path;
  };
  return base::ranges::equal(arg, expected, equal);
}

class PlaylistDownloadRequestManagerBrowserTest : public PlatformBrowserTest {
 public:
  PlaylistDownloadRequestManagerBrowserTest() {
    playlist::PlaylistDownloadRequestManager::SetPlaylistJavaScriptWorldId(
        ISOLATED_WORLD_ID_CHROME_INTERNAL);
  }
  ~PlaylistDownloadRequestManagerBrowserTest() override = default;

  void LoadHTMLAndCheckResult(
      const std::string& html,
      const std::vector<playlist::PlaylistItemInfo>& items) {
    const auto* test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    VLOG(2) << test_info->name() << ": " << __func__;

    if (embedded_test_server()->Started())
      ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&PlaylistDownloadRequestManagerBrowserTest::Serve,
                            base::Unretained(this), html));
    ASSERT_TRUE(embedded_test_server()->Start());

    // Load given |html| contents.
    const GURL url = embedded_test_server()->GetURL("/test");
    auto* active_web_contents = chrome_test_utils::GetActiveWebContents(this);

    ASSERT_TRUE(content::NavigateToURL(active_web_contents, url));

    // Run script and find media files
    ASSERT_FALSE(component_manager_->script().empty());
    playlist::PlaylistDownloadRequestManager::Request request;
    request.url_or_contents = active_web_contents->GetWeakPtr();
    request.callback =
        base::BindOnce(&PlaylistDownloadRequestManagerBrowserTest::OnGetMedia,
                       base::Unretained(this), test_info->name(), items);
    request_manager_->GetMediaFilesFromPage(std::move(request));

    // Block until result is received from OnGetMedia().
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

 protected:
  // PlatformBrowserTest:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    component_manager_ =
        std::make_unique<playlist::MediaDetectorComponentManager>(nullptr);
    component_manager_->SetUseLocalScriptForTesting();

    auto* profile = chrome_test_utils::GetProfile(this);
    request_manager_ =
        std::make_unique<playlist::PlaylistDownloadRequestManager>(
            profile, HostContentSettingsMapFactory::GetForProfile(profile),
            component_manager_.get());
  }

  void TearDownOnMainThread() override {
    request_manager_.reset();
    component_manager_.reset();

    PlatformBrowserTest::TearDownOnMainThread();
  }

 protected:
  void CreateBackgroundWebContents() { request_manager_->CreateWebContents(); }

  content::WebContents* GetBackgroundWebContents() {
    return request_manager_->web_contents_.get();
  }

  std::unique_ptr<net::test_server::HttpResponse> Serve(
      const std::string& html,
      const net::test_server::HttpRequest& request) {
    GURL absolute_url = embedded_test_server()->GetURL(request.relative_url);
    if (absolute_url.path() != "/test")
      return {};

    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content(html);
    response->set_content_type("text/html; charset=utf-8");
    return response;
  }

  void OnGetMedia(const char* test_name,
                  const std::vector<playlist::PlaylistItemInfo>& expected_items,
                  const std::vector<playlist::PlaylistItemInfo>& actual_items) {
    VLOG(2) << test_name << ": " << __func__;

    EXPECT_EQ(actual_items.size(), expected_items.size());

    auto path_fixer = [this](auto& item) {
      if (!item.media_file_path.empty()) {
        item.media_file_path =
            embedded_test_server()->GetURL(item.media_file_path).spec();
      }

      if (!item.thumbnail_path.empty()) {
        item.thumbnail_path =
            embedded_test_server()->GetURL(item.thumbnail_path).spec();
      }
    };
    auto comparer = [](const auto& a, const auto& b) {
      return a.media_file_path < b.media_file_path;
    };
    std::vector sorted_expected(expected_items.begin(), expected_items.end());
    base::ranges::for_each(sorted_expected, path_fixer);
    base::ranges::sort(sorted_expected, comparer);

    std::vector sorted_actual(actual_items.begin(), actual_items.end());
    base::ranges::sort(sorted_actual, comparer);
    EXPECT_THAT(sorted_actual, IsSamePlaylistItems(sorted_expected));

    ASSERT_TRUE(run_loop_);
    run_loop_->Quit();
  }

  std::unique_ptr<playlist::MediaDetectorComponentManager> component_manager_;
  std::unique_ptr<playlist::PlaylistDownloadRequestManager> request_manager_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

// Create background web contents and check PageSpecificContentSettings is
// attached to it.
IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       BackgroundWebContents) {
  CreateBackgroundWebContents();
  EXPECT_TRUE(
      content_settings::PageSpecificContentSettings::GetDelegateForWebContents(
          GetBackgroundWebContents()));
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest, NoMedia) {
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
        </body></html>
      )html",
      {});
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       SrcAttributeTest) {
  using playlist::PlaylistItemInfo;
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
          <video src="test.mp4"/>
        </body></html>
      )html",
      {
          {PlaylistItemInfo::Title(""), PlaylistItemInfo::ThumbnailPath(""),
           PlaylistItemInfo::MediaFilePath("/test.mp4")},
      });
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       SrcElementTest) {
  using playlist::PlaylistItemInfo;
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
          <video>
            <source src="test1.mp4"/>
            <source src="test2.mp4"/>
          </video>
        </body></html>
      )html",
      {
          {PlaylistItemInfo::Title(""), PlaylistItemInfo::ThumbnailPath(""),
           PlaylistItemInfo::MediaFilePath("/test1.mp4")},
          {PlaylistItemInfo::Title(""), PlaylistItemInfo::ThumbnailPath(""),
           PlaylistItemInfo::MediaFilePath("/test2.mp4")},
      });
}

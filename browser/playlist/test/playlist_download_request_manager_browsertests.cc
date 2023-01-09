/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_download_request_manager.h"

#include "base/ranges/algorithm.h"
#include "brave/components/playlist/media_detector_component_manager.h"
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

class PlaylistDownloadRequestManagerBrowserTest : public PlatformBrowserTest {
 public:
  struct ExpectedData {
    std::string name;
    std::string thumbnail_source;
    std::string media_source;
  };

  PlaylistDownloadRequestManagerBrowserTest() {
    playlist::PlaylistDownloadRequestManager::SetPlaylistJavaScriptWorldId(
        ISOLATED_WORLD_ID_CHROME_INTERNAL);
  }
  ~PlaylistDownloadRequestManagerBrowserTest() override = default;

  playlist::mojom::PlaylistItemPtr CreateItem(const ExpectedData& data) {
    auto item = playlist::mojom::PlaylistItem::New();
    item->name = data.name;
    item->thumbnail_source = GURL(data.thumbnail_source);
    item->thumbnail_path = GURL(data.thumbnail_source);
    item->media_source = GURL(data.media_source);
    item->media_path = GURL(data.media_source);
    return item;
  }

  void LoadHTMLAndCheckResult(const std::string& html,
                              const std::vector<ExpectedData>& items) {
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
    ASSERT_FALSE(component_manager_->GetMediaDetectorScript().empty());
    playlist::PlaylistDownloadRequestManager::Request request;
    request.url_or_contents = active_web_contents->GetWeakPtr();
    request.callback = base::BindOnce(
        &PlaylistDownloadRequestManagerBrowserTest::OnGetMedia,
        base::Unretained(this), test_info->name(), std::move(items));
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

    request_manager_ =
        std::make_unique<playlist::PlaylistDownloadRequestManager>(
            chrome_test_utils::GetProfile(this), component_manager_.get());
  }

  void TearDownOnMainThread() override {
    request_manager_.reset();
    component_manager_.reset();

    ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());

    PlatformBrowserTest::TearDownOnMainThread();
  }

 private:
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
                  std::vector<ExpectedData> expected_data,
                  std::vector<playlist::mojom::PlaylistItemPtr> actual_items) {
    VLOG(2) << test_name << ": " << __func__;

    std::vector<playlist::mojom::PlaylistItemPtr> expected_items;
    base::ranges::for_each(expected_data, [&](auto& item) {
      if (!item.thumbnail_source.empty()) {
        item.thumbnail_source =
            embedded_test_server()->GetURL(item.thumbnail_source).spec();
      }

      if (!item.media_source.empty()) {
        item.media_source =
            embedded_test_server()->GetURL(item.media_source).spec();
      }
      expected_items.push_back(CreateItem(item));
    });

    EXPECT_EQ(actual_items.size(), expected_items.size());

    auto equal = [](const auto& a, const auto& b) {
      // id is not compared because it is generated for actual items.
      return a->media_path == b->media_path && a->name == b->name &&
             a->thumbnail_path == b->thumbnail_path;
    };
    EXPECT_TRUE(base::ranges::equal(actual_items, expected_items, equal));

    ASSERT_TRUE(run_loop_);
    run_loop_->Quit();
  }

  std::unique_ptr<playlist::MediaDetectorComponentManager> component_manager_;
  std::unique_ptr<playlist::PlaylistDownloadRequestManager> request_manager_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

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
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
          <video src="test.mp4"/>
        </body></html>
      )html",
      {{.name = "", .thumbnail_source = "", .media_source = "/test.mp4"}});
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       SrcElementTest) {
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
          <video>
            <source src="test1.mp4"/>
            <source src="test2.mp4"/>
          </video>
        </body></html>
      )html",
      {{.name = "", .thumbnail_source = "", .media_source = "/test1.mp4"},
       {.name = "", .thumbnail_source = "", .media_source = "/test2.mp4"}});
}

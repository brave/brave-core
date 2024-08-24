/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/playlist/playlist_add_bubble_view.h"
#include "brave/browser/ui/views/playlist/playlist_bubble_view.h"
#include "brave/browser/ui/views/playlist/playlist_bubbles_controller.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/views/view_utils.h"

namespace playlist {
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

  PlaylistBubbleView* GetBubble() {
    auto* web_contents = GetActiveWebContents();
    if (!web_contents) {
      return nullptr;
    }

    auto* controller = PlaylistBubblesController::FromWebContents(web_contents);
    return controller ? controller->GetBubble() : nullptr;
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

  void ActivatePlaylistSidePanel() {
    auto* sidebar_controller =
        static_cast<BraveBrowser*>(browser())->sidebar_controller();
    sidebar_controller->ActivatePanelItem(
        sidebar::SidebarItem::BuiltInItemType::kPlaylist);
  }

  content::WebContents* GetPlaylistWebContents() {
    content::WebContents* contents = nullptr;

    // Wrap routine with lambda as ASSERT_FOO has return type internally.
    ([&]() {
      auto* coordinator = PlaylistSidePanelCoordinator::FromBrowser(browser());
      ASSERT_TRUE(coordinator);

      auto* contents_wrapper = coordinator->contents_wrapper();
      ASSERT_TRUE(coordinator);

      contents = contents_wrapper->web_contents();
      ASSERT_TRUE(contents);
    })();

    return contents;
  }

  virtual void SetUpHTTPSServer() {
    base::FilePath test_data_dir;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());
  }

  // PlatformBrowserTest:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    SetUpHTTPSServer();

    auto* service = GetService();
    ASSERT_TRUE(service);
    service->SetUpForTesting();
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

 protected:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;

 private:
  std::unique_ptr<base::RunLoop> run_loop_;

  base::test::ScopedFeatureList scoped_feature_list_;

  content::ContentMockCertVerifier mock_cert_verifier_;
};

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, AddItemsToList) {
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GetURL("/playlist/site_with_video.html")));

  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* location_bar_view = views::AsViewClass<BraveLocationBarView>(
      browser_view->GetLocationBarView());
  auto* playlist_action_icon_view =
      location_bar_view->GetPlaylistActionIconView();
  ASSERT_TRUE(playlist_action_icon_view);
  // Checks if PageActionIconView shows up on a site with videos.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_action_icon_view->GetVisible(); }));

  // The test page is simple video url. So we expect it to be found without
  // necessity of extracting media from background web contents.
  auto* tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());
  ASSERT_EQ(tab_helper->found_items().size(), 1u);
  ASSERT_FALSE(tab_helper->found_items()[0]->is_blob_from_media_source);

  // Show up bubble and add all found items.
  location_bar_view->ShowPlaylistBubble();
  PlaylistBubbleView* action_bubble = nullptr;
  WaitUntil(base::BindLambdaForTesting([&]() {
    action_bubble = GetBubble();
    return !!action_bubble;
  }));

  auto* add_bubble = views::AsViewClass<PlaylistAddBubbleView>(action_bubble);
  ASSERT_TRUE(add_bubble);
  // As we don't have to extract media from background web contents, spinner
  // shouldn't appear and items should be visible right away.
  EXPECT_FALSE(add_bubble->loading_spinner_->GetVisible());
  EXPECT_TRUE(add_bubble->scroll_view_->GetVisible());
  auto selected_items = add_bubble->list_view_->GetSelected();
  EXPECT_EQ(selected_items.size(), tab_helper->found_items().size());
  EXPECT_EQ(selected_items.size(), 1u);
  EXPECT_EQ(selected_items.front()->media_source,
            tab_helper->found_items().front()->media_source);

  action_bubble->Accept();

  // Checks if the added items are shown on playlist web ui.
  ActivatePlaylistSidePanel();
  auto* playlist_web_contents = GetPlaylistWebContents();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !playlist_web_contents->IsLoading(); }));

  ASSERT_TRUE(content::ExecJs(
      playlist_web_contents,
      "document.querySelector(`[class^='PlaylistCard']`).click();"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    return content::EvalJs(
               playlist_web_contents,
               "!!document.querySelector(`[class^='PlaylistItemContainer']`);")
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

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_PlayWithoutLocalCache) {
  // TODO(sko) This test is disabled for now. We haven't decide what the UI or
  // flow should be for this action.
  // Create an item and wait for it to be cached.
  ASSERT_TRUE(content::NavigateToURL(
      GetActiveWebContents(),
      https_server()->GetURL("test.googlevideo.com",
                             "/playlist/site_with_video.html")));

  ActivatePlaylistSidePanel();
  auto* playlist_web_contents = GetPlaylistWebContents();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !playlist_web_contents->IsLoading(); }));

  ASSERT_TRUE(content::ExecJs(
      playlist_web_contents,
      "document.querySelector('#download-from-active-tab-btn').click();"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    auto result = content::EvalJs(playlist_web_contents,
                                  R"-js(
          const item = document.querySelector(`[class^='PlaylistItemContainer']`);
          item && item.parentElement.parentElement
              .querySelector('.playlist-item-cached-state')
              .textContent == 'Cached';
        )-js");

    return !result.value.is_none() && result.ExtractBool();
  }));

  // Remove cache
  ASSERT_TRUE(content::ExecJs(playlist_web_contents,
                              R"-js(
          const item = document.querySelector(`[class^='PlaylistItemContainer']`);
          item.parentElement.parentElement
              .querySelector('.playlist-item-cache-btn').click();
        )-js"));
  WaitUntil(base::BindLambdaForTesting([&]() {
    auto result = content::EvalJs(playlist_web_contents,
                                  R"-js(
          const item = document.querySelector(`[class^='PlaylistItemContainer']`);
          item && item.parentElement.parentElement
              .querySelector('.playlist-item-cached-state')
              .textContent != 'Cached';
       )-js");

    return !result.value.is_none() && result.ExtractBool();
  }));

  // Try playing the item
  ASSERT_TRUE(content::ExecJs(playlist_web_contents,
                              R"-js(
          document.querySelector(`[class^='StyledThumbnail'], [class^='DefaultThumbnail']`).click();
        )-js"));

  WaitUntil(base::BindLambdaForTesting([&]() {
    return content::EvalJs(playlist_web_contents,
                           R"-js(
          document.querySelector(`#player`)
          .getAttribute('data-playing') === 'true';
        )-js")
        .ExtractBool();
  }));
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, PlaylistTabHelper) {
  auto* playlist_tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());
  EXPECT_TRUE(playlist_tab_helper);
  EXPECT_TRUE(playlist_tab_helper->found_items().empty());

  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GetURL("/playlist/site_with_video.html")));

  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_tab_helper->found_items().size() > 0; }));

  ASSERT_TRUE(content::NavigateToURL(
      GetActiveWebContents(), GetURL("/playlist/site_without_video.html")));
  // items should be cleared right away.
  EXPECT_TRUE(playlist_tab_helper->found_items().empty());

  // 'Back' should be observed
  browser()->command_controller()->ExecuteCommand(IDC_BACK);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_tab_helper->found_items().size() > 0; }));

  // Newly added items should be observed
  std::vector<playlist::mojom::PlaylistItemPtr> items_to_add;
  items_to_add.push_back(playlist_tab_helper->found_items().front()->Clone());
  GetService()->AddMediaFiles(std::move(items_to_add),
                              playlist::kDefaultPlaylistID,
                              /* can_cache= */ false, base::DoNothing());
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_tab_helper->saved_items().size() > 0; }));

  // Removed items should be observed
  GetService()->ResetAll();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_tab_helper->saved_items().size() == 0; }));

  // 'Forward' should be observed
  browser()->command_controller()->ExecuteCommand(IDC_FORWARD);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_tab_helper->found_items().size() == 0; }));
}

class PlaylistBrowserTestWithSitesUsingMediaSource
    : public PlaylistBrowserTest {
 public:
  void SetHTMLContents(const std::string& contents) { contents_ = contents; }

  // PlaylistBrowserTest:
  void SetUpHTTPSServer() override {
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &PlaylistBrowserTestWithSitesUsingMediaSource::Serve,
        base::Unretained(this)));
    ASSERT_TRUE(https_server_->Start());
  }

 private:
  std::unique_ptr<net::test_server::HttpResponse> Serve(
      const net::test_server::HttpRequest& request) {
    GURL absolute_url = https_server_->GetURL(request.relative_url);
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content(contents_);
    response->set_content_type("text/html; charset=utf-8");
    return response;
  }

  std::string contents_;
};

IN_PROC_BROWSER_TEST_F(
    PlaylistBrowserTestWithSitesUsingMediaSource,
    MediaShouldBeExtractedFromBackground_SucceedInExtracting) {
  SetHTMLContents(R"html(
        <html>
        <meta property="og:image" content="/img.jpg">
        <body>
          <video id="vid"/>
        </body>
        <script>
          if (window.MediaSource) {
            const videoElement = document.querySelector('#vid');
            videoElement.src = URL.createObjectURL(new MediaSource());
          } else {
            const videoElement = document.querySelector('#vid');
            videoElement.src = '/test.mp4';
          }
        </script>
        </html>
      )html");

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* location_bar_view = views::AsViewClass<BraveLocationBarView>(
      browser_view->GetLocationBarView());
  auto* playlist_action_icon_view =
      location_bar_view->GetPlaylistActionIconView();
  auto* playlist_tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());

  EXPECT_FALSE(playlist_action_icon_view->GetVisible());
  const GURL url = https_server()->GetURL("www.youtube.com", "/watch?v=12345");
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(), url));
  WaitUntil(base::BindLambdaForTesting(
      [&] { return playlist_action_icon_view->GetVisible(); }));

  EXPECT_EQ(playlist_tab_helper->found_items().size(), 1u);
  EXPECT_TRUE(playlist_tab_helper->found_items()[0]->is_blob_from_media_source);

  playlist_action_icon_view->ShowPlaylistBubble();
  auto* add_bubble = views::AsViewClass<PlaylistAddBubbleView>(GetBubble());
  EXPECT_TRUE(add_bubble);
  add_bubble->Accept();

  WaitUntil(base::BindLambdaForTesting([&] {
    auto* bubble = GetBubble();
    return bubble && !views::IsViewClass<PlaylistAddBubbleView>(bubble);
  }));

  EXPECT_EQ(playlist_tab_helper->saved_items().size(), 1u);
  EXPECT_FALSE(
      playlist_tab_helper->saved_items()[0]->is_blob_from_media_source);
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTestWithSitesUsingMediaSource,
                       MediaShouldBeExtractedFromBackground_FailToExtract) {
  SetHTMLContents(R"html(
        <html>
        <meta property="og:image" content="/img.jpg">
        <body>
          <video id="vid"/>
        </body>
        <script>
          if (window.MediaSource) {
            const videoElement = document.querySelector('#vid');
            videoElement.src = URL.createObjectURL(new MediaSource());
          }
        </script>
        </html>
      )html");

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* location_bar_view = views::AsViewClass<BraveLocationBarView>(
      browser_view->GetLocationBarView());
  auto* playlist_action_icon_view =
      location_bar_view->GetPlaylistActionIconView();
  auto* playlist_tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());

  EXPECT_FALSE(playlist_action_icon_view->GetVisible());
  const GURL url = https_server()->GetURL("www.youtube.com", "/watch?v=12345");
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(), url));
  WaitUntil(base::BindLambdaForTesting(
      [&] { return playlist_action_icon_view->GetVisible(); }));

  EXPECT_EQ(playlist_tab_helper->found_items().size(), 1u);
  EXPECT_TRUE(playlist_tab_helper->found_items()[0]->is_blob_from_media_source);

  playlist_action_icon_view->ShowPlaylistBubble();
  auto* add_bubble = views::AsViewClass<PlaylistAddBubbleView>(GetBubble());
  EXPECT_TRUE(add_bubble);
  add_bubble->Accept();
  EXPECT_TRUE(add_bubble->loading_spinner_->GetVisible());

  WaitUntil(base::BindLambdaForTesting([&] {
    auto* add_bubble = views::AsViewClass<PlaylistAddBubbleView>(GetBubble());
    return add_bubble ? !add_bubble->loading_spinner_->GetVisible() : false;
  }));

  EXPECT_TRUE(playlist_tab_helper->saved_items().empty());
}

IN_PROC_BROWSER_TEST_F(
    PlaylistBrowserTestWithSitesUsingMediaSource,
    MediaShouldBeExtractedFromBackground_DynamicallyAddedMedia) {
  SetHTMLContents(R"html(
        <html>
        <meta property="og:image" content="/img.jpg">
        <body>
          <video id="vid"/>
        </body>
        <script>
          if (window.MediaSource) {
            const videoElement = document.querySelector('#vid');
            videoElement.src = URL.createObjectURL(new MediaSource());
          } else {
            setTimeout(() => {
              const videoElement = document.querySelector('#vid');
              videoElement.src = '/test.mp4';
            }, 3000);
          }
        </script>
        </html>
      )html");

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* location_bar_view = views::AsViewClass<BraveLocationBarView>(
      browser_view->GetLocationBarView());
  auto* playlist_action_icon_view =
      location_bar_view->GetPlaylistActionIconView();
  auto* playlist_tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());

  EXPECT_FALSE(playlist_action_icon_view->GetVisible());
  const GURL url = https_server()->GetURL("www.ted.com", "/v12345");
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(), url));
  WaitUntil(base::BindLambdaForTesting(
      [&] { return playlist_action_icon_view->GetVisible(); }));

  EXPECT_EQ(playlist_tab_helper->found_items().size(), 1u);
  EXPECT_TRUE(playlist_tab_helper->found_items()[0]->is_blob_from_media_source);

  playlist_action_icon_view->ShowPlaylistBubble();
  auto* add_bubble = views::AsViewClass<PlaylistAddBubbleView>(GetBubble());
  EXPECT_TRUE(add_bubble);
  add_bubble->Accept();

  WaitUntil(base::BindLambdaForTesting([&] {
    auto* bubble = GetBubble();
    return bubble && !views::IsViewClass<PlaylistAddBubbleView>(bubble);
  }));

  EXPECT_EQ(playlist_tab_helper->saved_items().size(), 1u);
  EXPECT_FALSE(
      playlist_tab_helper->saved_items()[0]->is_blob_from_media_source);
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTestWithSitesUsingMediaSource,
                       AddMediaFiles_WithMediaSourceItem) {
  SetHTMLContents(R"html(
        <html>
        <meta property="og:image" content="/img.jpg">
        <body>
          <video id="vid"/>
        </body>
        <script>
          if (window.MediaSource) {
            const videoElement = document.querySelector('#vid');
            videoElement.src = URL.createObjectURL(new MediaSource());
          } else {
            const videoElement = document.querySelector('#vid');
            videoElement.src = '/test.mp4';
          }
        </script>
        </html>
      )html");

  const GURL url = https_server()->GetURL("www.youtube.com", "/watch?v=12345");
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(), url));

  auto* playlist_tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());
  WaitUntil(base::BindLambdaForTesting(
      [&] { return playlist_tab_helper->found_items().size() == 1; }));
  EXPECT_TRUE(playlist_tab_helper->found_items()[0]->is_blob_from_media_source);

  base::RunLoop run_loop;
  base::MockCallback<playlist::PlaylistService::AddMediaFilesCallback> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](std::vector<playlist::mojom::PlaylistItemPtr> items) {
        EXPECT_EQ(items.size(), 1u);
        EXPECT_FALSE(items[0]->is_blob_from_media_source);
        EXPECT_EQ(items[0]->parents.size(), 1u);
        EXPECT_EQ(items[0]->parents[0], playlist::kDefaultPlaylistID);
        run_loop.Quit();
      });

  std::vector<playlist::mojom::PlaylistItemPtr> items;
  base::ranges::transform(playlist_tab_helper->found_items(),
                          std::back_inserter(items),
                          &playlist::mojom::PlaylistItemPtr::Clone);

  auto* playlist_service = GetService();
  ASSERT_TRUE(playlist_service);
  playlist_service->AddMediaFiles(std::move(items),
                                  playlist::kDefaultPlaylistID,
                                  /* can_cache= */ false, callback.Get());

  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(
    PlaylistBrowserTestWithSitesUsingMediaSource,
    AddMediaFilesFromActiveTabToPlaylist_WithMediaSourceItem) {
  SetHTMLContents(R"html(
        <html>
        <meta property="og:image" content="/img.jpg">
        <body>
          <video id="vid"/>
        </body>
        <script>
          if (window.MediaSource) {
            const videoElement = document.querySelector('#vid');
            videoElement.src = URL.createObjectURL(new MediaSource());
          } else {
            const videoElement = document.querySelector('#vid');
            videoElement.src = '/test.mp4';
          }
        </script>
        </html>
      )html");

  const GURL url = https_server()->GetURL("www.youtube.com", "/watch?v=12345");
  ASSERT_TRUE(content::NavigateToURL(GetActiveWebContents(), url));

  auto* playlist_tab_helper =
      playlist::PlaylistTabHelper::FromWebContents(GetActiveWebContents());
  WaitUntil(base::BindLambdaForTesting(
      [&] { return playlist_tab_helper->found_items().size() == 1; }));
  EXPECT_TRUE(playlist_tab_helper->found_items()[0]->is_blob_from_media_source);

  base::RunLoop run_loop;
  base::MockCallback<
      playlist::PlaylistService::AddMediaFilesFromActiveTabToPlaylistCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](std::vector<playlist::mojom::PlaylistItemPtr> items) {
        EXPECT_EQ(items.size(), 1u);
        EXPECT_FALSE(items[0]->is_blob_from_media_source);
        EXPECT_EQ(items[0]->parents.size(), 1u);
        EXPECT_EQ(items[0]->parents[0], playlist::kDefaultPlaylistID);
        run_loop.Quit();
      });

  auto* playlist_service = GetService();
  ASSERT_TRUE(playlist_service);
  playlist_service->AddMediaFilesFromActiveTabToPlaylist(
      playlist::kDefaultPlaylistID, false, callback.Get());

  run_loop.Run();
}
}  // namespace playlist

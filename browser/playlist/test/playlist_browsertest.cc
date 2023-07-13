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
#include "brave/browser/playlist/playlist_tab_helper.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/browser/ui/views/playlist/playlist_action_bubble_view.h"
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_download_request_manager.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/views/view_utils.h"

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

  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* location_bar_view = views::AsViewClass<BraveLocationBarView>(
      browser_view->GetLocationBarView());
  auto* playlist_action_icon_view =
      location_bar_view->GetPlaylistActionIconView();
  ASSERT_TRUE(playlist_action_icon_view);
  // Checks if PageActionIconView shows up on a site with videos.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return playlist_action_icon_view->GetVisible(); }));

  // Show up bubble and add all found items.
  location_bar_view->ShowPlaylistBubble();
  PlaylistActionBubbleView* action_bubble = nullptr;
  WaitUntil(base::BindLambdaForTesting([&]() {
    action_bubble = PlaylistActionBubbleView::GetBubble();
    return !!action_bubble;
  }));
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

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <optional>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/synchronization/lock.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/peek/peek_features.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "brave/browser/ui/views/peek/peek_controller_impl.h"
#include "brave/browser/ui/views/peek/peek_shields_button.h"
#include "brave/browser/ui/views/peek/peek_view.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ssl/chrome_security_state_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view_base.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/permissions/permission_request_manager.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/security_state/core/security_state.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#endif

class PeekBrowserTest : public InProcessBrowserTest {
 public:
  PeekBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_features_.InitAndEnableFeature(peek::features::kBravePeek);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());

    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &PeekBrowserTest::MonitorRequest, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());

    // Peek only opens from http(s) pages.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                             GetURL("a.com", "/title1.html")));
  }

 protected:
  GURL GetURL(const std::string& host, const std::string& path) {
    return embedded_test_server()->GetURL(host, path);
  }

  TabStripModel* tab_strip_model() { return browser()->tab_strip_model(); }

  content::WebContents* active_contents() {
    return tab_strip_model()->GetActiveWebContents();
  }

  PeekControllerImpl* controller() {
    return static_cast<PeekControllerImpl*>(PeekController::From(browser()));
  }

  content::WebContents* peek_contents() {
    return controller()->GetWebContents();
  }

  // Params for a link click on the active tab's page.
  content::OpenURLParams LinkParams(const GURL& url) {
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                  ui::PAGE_TRANSITION_LINK,
                                  /*is_renderer_initiated=*/true);
    params.initiator_origin =
        active_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
    return params;
  }

  // Opens `url` in Peek and waits for it to load.
  void OpenPeek(const GURL& url) {
    content::TestNavigationObserver observer(url);
    observer.StartWatchingNewWebContents();
    ASSERT_TRUE(controller()->Open(*active_contents(), LinkParams(url)));
    observer.Wait();
    ASSERT_TRUE(controller()->IsShowing());
  }

  void WaitForPeekClosed() {
    ASSERT_TRUE(
        base::test::RunUntil([&] { return !controller()->IsShowing(); }));
  }

  // Waits until Peek closes and `contents` becomes the active tab.
  void WaitForMovedToTab(content::WebContents* contents) {
    WaitForPeekClosed();
    EXPECT_EQ(contents, active_contents());
  }

  // Adds a link covering the top-left of the page and Shift+clicks it.
  void ShiftClickLink(content::WebContents* contents, const GURL& url) {
    ASSERT_TRUE(content::ExecJs(contents, content::JsReplace(R"(
          const a = document.createElement('a');
          a.id = 'link';
          a.href = $1;
          a.style = 'position:absolute;left:0;top:0;width:100px;' +
                    'height:100px;display:block';
          document.body.appendChild(a);
        )",
                                                             url)));
    content::SimulateEndOfPaintHoldingOnPrimaryMainFrame(contents);
    content::SimulateMouseClickAt(
        contents, blink::WebInputEvent::kShiftKey,
        blink::WebMouseEvent::Button::kLeft,
        gfx::ToFlooredPoint(
            content::GetCenterCoordinatesOfElementWithId(contents, "link")));
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateLinkContextMenu(
      content::WebContents* contents,
      const GURL& link_url) {
    const GURL page_url = contents->GetLastCommittedURL();
    content::ContextMenuParams params;
    params.page_url = page_url;
    params.frame_url = page_url;
    params.frame_origin =
        contents->GetPrimaryMainFrame()->GetLastCommittedOrigin();
    params.link_url = link_url;
    params.unfiltered_link_url = link_url;
    auto menu = std::make_unique<TestRenderViewContextMenu>(
        *contents->GetPrimaryMainFrame(), params);
    menu->Init();
    return menu;
  }

  // Returns the headers of the request for `relative_url`, if one was seen.
  std::optional<net::test_server::HttpRequest::HeaderMap> GetRequestHeaders(
      const std::string& relative_url) {
    base::AutoLock lock(lock_);
    auto it = request_headers_.find(relative_url);
    if (it == request_headers_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  static std::string GetHeader(
      const net::test_server::HttpRequest::HeaderMap& headers,
      const std::string& name) {
    auto it = headers.find(name);
    return it == headers.end() ? std::string() : it->second;
  }

  void ExpectCrossSiteRequestMetadata(const std::string& path) {
    std::optional<net::test_server::HttpRequest::HeaderMap> headers;
    ASSERT_TRUE(base::test::RunUntil([&] {
      headers = GetRequestHeaders(path);
      return headers.has_value();
    }));
    EXPECT_EQ("cross-site", GetHeader(*headers, "Sec-Fetch-Site"));
    EXPECT_EQ("navigate", GetHeader(*headers, "Sec-Fetch-Mode"));
    const std::string cookies = GetHeader(*headers, "Cookie");
    EXPECT_NE(std::string::npos, cookies.find("lax=1")) << cookies;
    EXPECT_EQ(std::string::npos, cookies.find("strict=1")) << cookies;
  }

  // Saves downloads to a temp dir without prompting.
  content::DownloadManager* SetUpDownloads() {
    {
      base::ScopedAllowBlockingForTesting allow_blocking;
      CHECK(downloads_dir_.CreateUniqueTempDir());
    }
    Profile* profile = browser()->GetProfile();
    profile->GetPrefs()->SetBoolean(prefs::kPromptForDownload, false);
    DownloadPrefs::FromBrowserContext(profile)->SetDownloadPath(
        downloads_dir_.GetPath());
    return profile->GetDownloadManager();
  }

  void SetSameSiteCookies(const GURL& url) {
    ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url,
                                   "strict=1; SameSite=Strict"));
    ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url,
                                   "lax=1; SameSite=Lax"));
  }

  net::EmbeddedTestServer https_server_;
  base::ScopedTempDir downloads_dir_;
  base::test::ScopedFeatureList scoped_features_;

 private:
  void MonitorRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock lock(lock_);
    request_headers_[request.relative_url] = request.headers;
  }

  base::Lock lock_;
  std::map<std::string, net::test_server::HttpRequest::HeaderMap>
      request_headers_ GUARDED_BY(lock_);
};

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ShiftClickOpensPeek) {
  const GURL target_url = GetURL("a.com", "/title2.html");
  content::WebContents* contents = active_contents();

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  ShiftClickLink(contents, target_url);
  observer.Wait();

  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(target_url, peek_contents()->GetLastCommittedURL());
  EXPECT_EQ(contents, active_contents());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ContextMenuOpensPeek) {
  const GURL target_url = GetURL("b.com", "/title2.html");
  auto menu = CreateLinkContextMenu(active_contents(), target_url);
  ASSERT_TRUE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKPEEK));
  EXPECT_TRUE(menu->IsItemEnabled(IDC_CONTENT_CONTEXT_OPENLINKPEEK));

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  menu->ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKPEEK, 0);
  observer.Wait();

  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(target_url, peek_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, EscapeClosesPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));
  content::SimulateKeyPress(peek_contents(), ui::DomKey::ESCAPE,
                            ui::DomCode::ESCAPE, ui::VKEY_ESCAPE,
                            /*control=*/false, /*shift=*/false, /*alt=*/false,
                            /*command=*/false);
  WaitForPeekClosed();
  EXPECT_FALSE(peek_contents());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, CloseButtonClosesPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  views::test::ButtonTestApi(
      controller()->peek_view_for_testing()->close_button_for_testing())
      .NotifyDefaultMouseClick();
  WaitForPeekClosed();
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, TabOutOfPageMovesFocusToToolbar) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  views::FocusManager* focus_manager =
      controller()->peek_view_for_testing()->GetFocusManager();
  ASSERT_TRUE(focus_manager);
  views::View* page_view = focus_manager->GetFocusedView();
  ASSERT_TRUE(page_view);

  // The page hands focus back when Tab or Shift+Tab leaves it. It must stay
  // inside Peek instead of being dropped.
  content::WebContents* contents = peek_contents();
  EXPECT_TRUE(contents->GetDelegate()->TakeFocus(contents, /*reverse=*/false));
  EXPECT_NE(page_view, focus_manager->GetFocusedView());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ClickOutsideCardClosesPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  PeekView* peek_view = controller()->peek_view_for_testing();
  ASSERT_GT(peek_view->card_for_testing()->x(), 0);
  const gfx::Point outside(1, 1);
  peek_view->OnMousePressed(ui::MouseEvent(
      ui::EventType::kMousePressed, outside, outside, ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  WaitForPeekClosed();
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenAsTabKeepsWebContents) {
  const GURL target_url = GetURL("b.com", "/title2.html");
  OpenPeek(target_url);
  content::WebContents* contents = peek_contents();
  const int tab_count = tab_strip_model()->count();

  views::test::ButtonTestApi(
      controller()->peek_view_for_testing()->open_as_tab_button_for_testing())
      .NotifyDefaultMouseClick();
  WaitForPeekClosed();

  EXPECT_EQ(tab_count + 1, tab_strip_model()->count());
  EXPECT_EQ(contents, active_contents());
  EXPECT_EQ(target_url, active_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, SwitchingTabsClosesPeek) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GetURL("a.com", "/title1.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  OpenPeek(GetURL("b.com", "/title2.html"));

  tab_strip_model()->ActivateTabAt(0);
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpeningAnotherPeekReplacesIt) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  const GURL second_url = GetURL("c.com", "/title2.html");
  OpenPeek(second_url);
  EXPECT_EQ(second_url, peek_contents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, SiteInfoButtonShowsPageInfoForPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  views::test::ButtonTestApi(
      controller()->peek_view_for_testing()->site_info_button_for_testing())
      .NotifyDefaultMouseClick();

  ASSERT_EQ(PageInfoBubbleViewBase::BUBBLE_PAGE_INFO,
            PageInfoBubbleViewBase::GetShownBubbleType());
  auto* bubble = views::AsViewClass<PageInfoBubbleViewBase>(
      PageInfoBubbleViewBase::GetPageInfoBubbleForTesting());
  ASSERT_TRUE(bubble);
  EXPECT_EQ(peek_contents(), bubble->web_contents());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ShieldsButtonShowsPanel) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  PeekShieldsButton* button =
      controller()->peek_view_for_testing()->shields_button_for_testing();
  views::test::ButtonTestApi(button).NotifyDefaultMouseClick();
  ASSERT_TRUE(button->GetBubbleWidget());

  // Pressing again while the panel is still loading doesn't cancel it.
  views::test::ButtonTestApi(button).NotifyDefaultMouseClick();
  ASSERT_TRUE(base::test::RunUntil([&] {
    return button->GetBubbleWidget() && button->GetBubbleWidget()->IsVisible();
  }));

  // Closing Peek also tears down the panel.
  controller()->Close();
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenGrowsCard) {
  auto rich_animation = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_ENABLED);
  ASSERT_TRUE(controller()->Open(*active_contents(),
                                 LinkParams(GetURL("b.com", "/title2.html"))));

  PeekView* peek_view = controller()->peek_view_for_testing();
  gfx::SlideAnimation& animation = peek_view->open_animation_for_testing();
  ASSERT_TRUE(animation.is_animating());
  views::test::RunScheduledLayout(peek_view);
  const int initial_width = peek_view->card_for_testing()->width();
  EXPECT_EQ(0, peek_view->card_for_testing()->y());

  ASSERT_TRUE(base::test::RunUntil([&] { return !animation.is_animating(); }));
  views::test::RunScheduledLayout(peek_view);
  EXPECT_GT(peek_view->card_for_testing()->width(), initial_width);
  EXPECT_EQ(0, peek_view->card_for_testing()->y());
}

// The card hangs from the toolbar, so page content can't pass itself off as
// Peek.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, CardIsAttachedToToolbar) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  views::View* contents_container =
      BrowserView::GetBrowserViewForBrowser(browser())->contents_container();
  EXPECT_EQ(contents_container->GetBoundsInScreen(),
            controller()->widget_for_testing()->GetWindowBoundsInScreen());

  PeekView* peek_view = controller()->peek_view_for_testing();
  views::test::RunScheduledLayout(peek_view);
  EXPECT_EQ(0, peek_view->card_for_testing()->y());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenRequiresHttpInitiatorAndTarget) {
  content::OpenURLParams no_initiator =
      LinkParams(GetURL("b.com", "/title2.html"));
  no_initiator.initiator_origin = std::nullopt;
  EXPECT_FALSE(controller()->Open(*active_contents(), no_initiator));

  content::OpenURLParams opaque_initiator =
      LinkParams(GetURL("b.com", "/title2.html"));
  opaque_initiator.initiator_origin = url::Origin();
  EXPECT_FALSE(controller()->Open(*active_contents(), opaque_initiator));

  EXPECT_FALSE(controller()->Open(*active_contents(),
                                  LinkParams(GURL("data:text/html,hi"))));
  EXPECT_FALSE(controller()->Open(*active_contents(),
                                  LinkParams(GURL("chrome://version"))));
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, NoPeekMenuItemOnBrowserPages) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://version")));
  auto menu =
      CreateLinkContextMenu(active_contents(), GetURL("b.com", "/title2.html"));
  EXPECT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKPEEK));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, NoNestedPeek) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  content::WebContents* contents = peek_contents();
  EXPECT_FALSE(controller()->CanOpenFrom(contents));

  auto menu = CreateLinkContextMenu(contents, GetURL("c.com", "/title2.html"));
  EXPECT_FALSE(menu->IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKPEEK));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, CannotOpenFromAnotherWindow) {
  BrowserWindowInterface* other = CreateBrowser(browser()->GetProfile());
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(other, GetURL("a.com", "/title1.html")));
  EXPECT_FALSE(controller()->CanOpenFrom(
      other->GetTabStripModel()->GetActiveWebContents()));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, BlocksNonHttpNavigations) {
  const GURL peek_url = GetURL("b.com", "/title1.html");
  OpenPeek(peek_url);
  content::WebContents* contents = peek_contents();

  // Top-level data: navigations from the renderer are already blocked before a
  // navigation starts, so they aren't covered here.
  for (const char* script : {
           "location.href = URL.createObjectURL(new Blob(['<p>hi</p>'], "
           "{type: 'text/html'}));",
           "location.href = 'about:blank';",
           "location.href = 'mailto:peek@example.com';",
       }) {
    SCOPED_TRACE(script);
    content::TestNavigationObserver observer(contents);
    ASSERT_TRUE(content::ExecJs(contents, script));
    observer.Wait();
    EXPECT_FALSE(observer.last_navigation_succeeded());
    EXPECT_EQ(peek_url, contents->GetLastCommittedURL());
  }
  EXPECT_TRUE(controller()->IsShowing());
}

// Server redirects to external protocols are allowed in tabs, so they have to
// be caught by the Peek throttle.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, BlocksRedirectToExternalProtocol) {
  const GURL peek_url = GetURL("b.com", "/title1.html");
  OpenPeek(peek_url);
  content::WebContents* contents = peek_contents();

  content::TestNavigationObserver observer(contents);
  ASSERT_TRUE(content::ExecJs(
      contents,
      content::JsReplace(
          "location.href = $1;",
          GetURL("b.com", "/server-redirect?mailto:peek@example.com"))));
  observer.Wait();
  EXPECT_FALSE(observer.last_navigation_succeeded());
  EXPECT_EQ(peek_url, contents->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PermissionRequestMovesToTab) {
  OpenPeek(https_server_.GetURL("b.test", "/title1.html"));
  content::WebContents* contents = peek_contents();
  auto* manager =
      permissions::PermissionRequestManager::FromWebContents(contents);
  ASSERT_TRUE(manager);

  ASSERT_TRUE(content::ExecJs(contents, "Notification.requestPermission();",
                              content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES));
  WaitForMovedToTab(contents);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return manager->GetCurrentPrompt(); }));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, FullscreenMovesToTab) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  content::WebContents* contents = peek_contents();

  ASSERT_TRUE(content::ExecJs(contents, "document.body.requestFullscreen();",
                              content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES));
  WaitForMovedToTab(contents);
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PopupWithGestureMovesToTab) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  content::WebContents* contents = peek_contents();
  const int tab_count = tab_strip_model()->count();

  const GURL popup_url = GetURL("b.com", "/title2.html");
  content::TestNavigationObserver popup_observer(popup_url);
  popup_observer.StartWatchingNewWebContents();
  ASSERT_TRUE(content::ExecJs(
      contents, content::JsReplace("window.open($1);", popup_url)));
  popup_observer.Wait();

  WaitForPeekClosed();
  // Both the promoted Peek and the popup are now tabs.
  EXPECT_EQ(tab_count + 2, tab_strip_model()->count());
  EXPECT_NE(TabStripModel::kNoTab,
            tab_strip_model()->GetIndexOfWebContents(contents));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PopupWithoutGestureIsBlocked) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  content::WebContents* contents = peek_contents();
  const int tab_count = tab_strip_model()->count();

  EXPECT_EQ(false,
            content::EvalJs(contents,
                            content::JsReplace("!!window.open($1);",
                                               GetURL("b.com", "/title2.html")),
                            content::EXECUTE_SCRIPT_NO_USER_GESTURE));
  EXPECT_TRUE(controller()->IsShowing());
  EXPECT_EQ(contents, peek_contents());
  EXPECT_EQ(tab_count, tab_strip_model()->count());
}

// Downloads are served over HTTPS so they aren't held back by insecure
// download warnings.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PeekedDownloadLinkClosesPeek) {
  content::DownloadManager* download_manager = SetUpDownloads();
  content::DownloadTestObserverTerminal download_observer(
      download_manager, 1,
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server_.GetURL("a.test", "/title1.html")));
  const int tab_count = tab_strip_model()->count();

  ASSERT_TRUE(controller()->Open(
      *active_contents(),
      LinkParams(https_server_.GetURL("b.test", "/downloads/a_zip_file.zip"))));

  WaitForPeekClosed();
  EXPECT_EQ(tab_count, tab_strip_model()->count());
  download_observer.WaitForFinished();
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, DownloadFromPeekedPageMovesToTab) {
  content::DownloadManager* download_manager = SetUpDownloads();
  content::DownloadTestObserverTerminal download_observer(
      download_manager, 1,
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);

  OpenPeek(https_server_.GetURL("b.test", "/title1.html"));
  content::WebContents* contents = peek_contents();
  ASSERT_TRUE(content::ExecJs(contents, R"(
        const a = document.createElement('a');
        a.href = '/downloads/a_zip_file.zip';
        a.download = '';
        document.body.appendChild(a);
        a.click();
      )"));

  WaitForMovedToTab(contents);
  download_observer.WaitForFinished();
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, HttpAuthMovesToTab) {
  const GURL auth_url = GetURL("b.com", "/auth-basic");
  ASSERT_TRUE(controller()->Open(*active_contents(), LinkParams(auth_url)));
  content::WebContents* contents = peek_contents();
  ASSERT_TRUE(contents);
  WaitForMovedToTab(contents);
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, HttpPageIsNotMarkedSecure) {
  const GURL url = GetURL("b.com", "/title2.html");
  OpenPeek(url);
  ASSERT_EQ(url, peek_contents()->GetLastCommittedURL());
  EXPECT_NE(security_state::SECURE,
            chrome_security_state::GetSecurityLevel(peek_contents()));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, HttpsPageIsMarkedSecure) {
  OpenPeek(https_server_.GetURL("b.test", "/title2.html"));
  EXPECT_EQ(security_state::SECURE,
            chrome_security_state::GetSecurityLevel(peek_contents()));
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, CertErrorShowsInterstitial) {
  net::EmbeddedTestServer bad_server(net::EmbeddedTestServer::TYPE_HTTPS);
  bad_server.SetSSLConfig(net::EmbeddedTestServer::CERT_MISMATCHED_NAME);
  bad_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(bad_server.Start());

  const GURL url = bad_server.GetURL("b.test", "/title2.html");
  content::TestNavigationObserver observer(url);
  observer.StartWatchingNewWebContents();
  ASSERT_TRUE(controller()->Open(*active_contents(), LinkParams(url)));
  observer.Wait();
  content::WebContents* contents = peek_contents();
  ASSERT_TRUE(contents);

  auto* helper =
      security_interstitials::SecurityInterstitialTabHelper::FromWebContents(
          contents);
  ASSERT_TRUE(helper);
  EXPECT_TRUE(helper->IsDisplayingInterstitial());
  EXPECT_EQ(security_state::DANGEROUS,
            chrome_security_state::GetSecurityLevel(contents));

  // Opening as a tab keeps the interstitial.
  controller()->OpenAsTab();
  EXPECT_EQ(contents, active_contents());
  EXPECT_TRUE(helper->IsDisplayingInterstitial());
}

// Peek navigations carry the opener's origin, so SameSite and Fetch Metadata
// treat them like a link click rather than a typed URL.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ShiftClickSendsInitiatorOrigin) {
  const std::string path = "/title2.html?shift-click";
  const GURL target_url = https_server_.GetURL("b.test", path);
  SetSameSiteCookies(target_url);

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  ShiftClickLink(active_contents(), target_url);
  observer.Wait();
  ASSERT_TRUE(controller()->IsShowing());

  ExpectCrossSiteRequestMetadata(path);
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ContextMenuSendsInitiatorOrigin) {
  const std::string path = "/title2.html?context-menu";
  const GURL target_url = https_server_.GetURL("b.test", path);
  SetSameSiteCookies(target_url);

  auto menu = CreateLinkContextMenu(active_contents(), target_url);
  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  menu->ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKPEEK, 0);
  observer.Wait();
  ASSERT_TRUE(controller()->IsShowing());

  ExpectCrossSiteRequestMetadata(path);
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpensFromSplitViewAsStandaloneTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GetURL("a.com", "/title1.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  tab_strip_model()->ActivateTabAt(0);
  tab_strip_model()->AddToNewSplit(
      {1}, split_tabs::SplitTabVisualData(),
      split_tabs::SplitTabCreatedSource::kTabContextMenu);
  ASSERT_TRUE(tab_strip_model()->GetActiveTab()->IsSplit());

  // Peek covers the whole split, not just the source pane.
  OpenPeek(GetURL("b.com", "/title2.html"));
  EXPECT_EQ(BrowserView::GetBrowserViewForBrowser(browser())
                ->contents_container()
                ->GetBoundsInScreen(),
            controller()->widget_for_testing()->GetWindowBoundsInScreen());

  content::WebContents* contents = peek_contents();
  controller()->OpenAsTab();
  EXPECT_EQ(contents, active_contents());
  EXPECT_FALSE(tab_strip_model()->GetActiveTab()->IsSplit());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, IncognitoPeekStaysOffTheRecord) {
  BrowserWindowInterface* incognito =
      CreateIncognitoBrowser(browser()->GetProfile());
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(incognito, GetURL("a.com", "/title1.html")));
  auto* incognito_controller =
      static_cast<PeekControllerImpl*>(PeekController::From(incognito));
  ASSERT_TRUE(incognito_controller);

  content::WebContents* source =
      incognito->GetTabStripModel()->GetActiveWebContents();
  const GURL url = GetURL("b.com", "/title2.html");
  content::OpenURLParams params(url, content::Referrer(),
                                WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                ui::PAGE_TRANSITION_LINK,
                                /*is_renderer_initiated=*/true);
  params.initiator_origin =
      source->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  content::TestNavigationObserver observer(url);
  observer.StartWatchingNewWebContents();
  ASSERT_TRUE(incognito_controller->Open(*source, params));
  observer.Wait();

  content::WebContents* contents = incognito_controller->GetWebContents();
  ASSERT_TRUE(contents);
  EXPECT_TRUE(contents->GetBrowserContext()->IsOffTheRecord());
  EXPECT_EQ(source->GetBrowserContext(), contents->GetBrowserContext());
}

#if BUILDFLAG(ENABLE_CONTAINERS)
class PeekContainersBrowserTest : public PeekBrowserTest {
 public:
  PeekContainersBrowserTest() {
    containers_features_.InitAndEnableFeature(
        containers::features::kContainers);
  }

 private:
  base::test::ScopedFeatureList containers_features_;
};

IN_PROC_BROWSER_TEST_F(PeekContainersBrowserTest, InheritsContainerPartition) {
  NavigateParams params(browser(), GetURL("a.com", "/title1.html"),
                        ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->GetProfile(), containers::kContainersStoragePartitionDomain,
      "peek-test-container", /*in_memory=*/false);
  ui_test_utils::NavigateToURL(&params);
  const content::StoragePartitionConfig source_config =
      active_contents()
          ->GetPrimaryMainFrame()
          ->GetStoragePartition()
          ->GetConfig();
  ASSERT_EQ(containers::kContainersStoragePartitionDomain,
            source_config.partition_domain());

  OpenPeek(GetURL("b.com", "/title2.html"));
  EXPECT_EQ(source_config, peek_contents()
                               ->GetPrimaryMainFrame()
                               ->GetStoragePartition()
                               ->GetConfig());
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

class PeekDisabledBrowserTest : public InProcessBrowserTest {
 public:
  PeekDisabledBrowserTest() {
    scoped_features_.InitAndDisableFeature(peek::features::kBravePeek);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(PeekDisabledBrowserTest,
                       LinkNavigatesAndHasNoPeekMenuItem) {
  EXPECT_FALSE(PeekController::From(browser()));

  const GURL page_url = embedded_test_server()->GetURL("a.com", "/title1.html");
  const GURL target_url =
      embedded_test_server()->GetURL("b.com", "/title2.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), page_url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  content::TestNavigationObserver observer(contents);
  ASSERT_TRUE(content::ExecJs(contents, content::JsReplace(R"(
        const a = document.createElement('a');
        a.href = $1;
        document.body.appendChild(a);
        a.click();
      )",
                                                           target_url)));
  observer.Wait();
  EXPECT_EQ(target_url, contents->GetLastCommittedURL());

  content::ContextMenuParams params;
  params.page_url = target_url;
  params.frame_url = target_url;
  params.link_url = page_url;
  TestRenderViewContextMenu menu(*contents->GetPrimaryMainFrame(), params);
  menu.Init();
  EXPECT_FALSE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKPEEK));
}

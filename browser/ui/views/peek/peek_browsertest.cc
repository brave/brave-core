/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/peek/peek_features.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "brave/browser/ui/views/peek/peek_controller_impl.h"
#include "brave/browser/ui/views/peek/peek_view.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/chrome_security_state_util.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/split_tab_metrics.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/input/native_web_keyboard_event.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/security_state/core/security_state.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
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
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
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
#endif

namespace {

// Records what the navigation to `url` knows about its initiator.
class NavigationInitiatorObserver : public content::WebContentsObserver {
 public:
  NavigationInitiatorObserver(content::WebContents* contents, const GURL& url)
      : content::WebContentsObserver(contents), url_(url) {}

  void Wait() { run_loop_.Run(); }

  bool has_initiator_navigation_state() const {
    return has_initiator_navigation_state_;
  }
  bool is_renderer_initiated() const { return is_renderer_initiated_; }
  const std::optional<url::Origin>& initiator_origin() const {
    return initiator_origin_;
  }

 private:
  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override {
    if (handle->GetURL() != url_ || !handle->IsInPrimaryMainFrame()) {
      return;
    }
    has_initiator_navigation_state_ = !!handle->GetInitiatorNavigationState();
    is_renderer_initiated_ = handle->IsRendererInitiated();
    initiator_origin_ = handle->GetInitiatorOrigin();
    run_loop_.Quit();
  }

  const GURL url_;
  base::RunLoop run_loop_;
  bool has_initiator_navigation_state_ = false;
  bool is_renderer_initiated_ = false;
  std::optional<url::Origin> initiator_origin_;
};

}  // namespace

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

  // Adds an http(s) tab after the others and activates it.
  content::WebContents* AddTab() {
    CHECK(ui_test_utils::NavigateToURLWithDisposition(
        browser(), GetURL("a.com", "/title1.html"),
        WindowOpenDisposition::kNewForegroundTab,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
    return active_contents();
  }

  PeekControllerImpl* controller() {
    return static_cast<PeekControllerImpl*>(PeekController::From(browser()));
  }

  content::WebContents* peek_contents() {
    return controller()->GetWebContents();
  }

  // Params for a link click in `frame`.
  static content::OpenURLParams LinkParamsFrom(content::RenderFrameHost& frame,
                                               const GURL& url) {
    content::OpenURLParams params =
        content::OpenURLParams::CreateRendererInitiated(
            url, WindowOpenDisposition::kNewForegroundTab,
            ui::PAGE_TRANSITION_LINK, content::Referrer(),
            frame.GetCurrentInitiatorNavigationState());
    params.initiator_origin = frame.GetLastCommittedOrigin();
    params.initiator_frame_token = frame.GetFrameToken();
    params.initiator_process_id = frame.GetProcess()->GetID().GetUnsafeValue();
    params.user_gesture = true;
    return params;
  }

  // Params for a link click on the active tab's page.
  content::OpenURLParams LinkParams(const GURL& url) {
    return LinkParamsFrom(*active_contents()->GetPrimaryMainFrame(), url);
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
  void WaitForMovedToTab(
      content::WebContents* contents,
      const base::Location& location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
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

  // Returns the headers of the request for `relative_url`, if one was seen.
  std::optional<net::test_server::HttpRequest::HeaderMap> GetRequestHeaders(
      const std::string& relative_url) {
    base::AutoLock lock(lock_);
    const auto* headers = base::FindOrNull(request_headers_, relative_url);
    return headers ? std::make_optional(*headers) : std::nullopt;
  }

  static std::string GetHeader(
      const net::test_server::HttpRequest::HeaderMap& headers,
      const std::string& name) {
    const auto* value = base::FindOrNull(headers, name);
    return value ? *value : std::string();
  }

  void ExpectCrossSiteRequestMetadata(
      const std::string& path,
      const base::Location& location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
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

  void SetSameSiteCookies(const GURL& url) {
    ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url,
                                   "strict=1; SameSite=Strict"));
    ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url,
                                   "lax=1; SameSite=Lax"));
  }

  net::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList scoped_features_;

 private:
  void MonitorRequest(const net::test_server::HttpRequest& request) {
    base::AutoLock lock(lock_);
    request_headers_[request.relative_url] = request.headers;
  }

  base::Lock lock_;
  base::flat_map<std::string, net::test_server::HttpRequest::HeaderMap>
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

// Escape is handled before the page gets it, so it works even if the page
// would consume it or its renderer never acks it.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, EscapeClosesPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));
  ASSERT_TRUE(content::ExecJs(peek_contents(), R"(
        addEventListener('keydown', e => e.preventDefault(), true);
      )"));
  // `content::SimulateKeyPress()` marks its events `skip_if_unhandled`, which
  // skips the delegate's pre-handling, so send the key the way the browser
  // window would.
  input::NativeWebKeyboardEvent event(
      blink::WebInputEvent::Type::kRawKeyDown,
      blink::WebInputEvent::kNoModifiers,
      blink::WebInputEvent::GetStaticTimeStampForTests());
  event.windows_key_code = ui::VKEY_ESCAPE;
  event.dom_key = ui::DomKey::ESCAPE;
  event.dom_code = static_cast<int>(ui::DomCode::ESCAPE);
  event.skip_if_unhandled = false;
  peek_contents()
      ->GetPrimaryMainFrame()
      ->GetRenderWidgetHost()
      ->ForwardKeyboardEvent(event);
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

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenInSplitViewSplitsWithSourceTab) {
  const GURL target_url = GetURL("b.com", "/title2.html");
  content::WebContents* source = active_contents();
  OpenPeek(target_url);
  content::WebContents* contents = peek_contents();
  views::ImageButton* split_view_button =
      controller()->peek_view_for_testing()->split_view_button_for_testing();
  ASSERT_TRUE(split_view_button->GetVisible());

  views::test::ButtonTestApi(split_view_button).NotifyDefaultMouseClick();
  WaitForMovedToTab(contents);

  // Moved without a reload, next to the source tab and in a split with it.
  EXPECT_EQ(target_url, contents->GetLastCommittedURL());
  EXPECT_EQ(2, tab_strip_model()->count());
  tabs::TabInterface* tab = tab_strip_model()->GetActiveTab();
  ASSERT_TRUE(tab->IsSplit());
  EXPECT_EQ(tab->GetSplit(),
            tabs::TabInterface::GetFromContents(source)->GetSplit());
}

// The source tab is the split's pivot, so the Peek page joins its pinned state.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenInSplitViewFromPinnedTab) {
  content::WebContents* source = active_contents();
  tab_strip_model()->SetTabPinned(0, true);
  AddTab();
  tab_strip_model()->ActivateTabAt(0);
  OpenPeek(GetURL("b.com", "/title2.html"));
  content::WebContents* contents = peek_contents();

  controller()->OpenInSplitView();
  EXPECT_EQ(contents, active_contents());
  tabs::TabInterface* tab = tab_strip_model()->GetActiveTab();
  ASSERT_TRUE(tab->IsSplit());
  EXPECT_EQ(tab->GetSplit(),
            tabs::TabInterface::GetFromContents(source)->GetSplit());
  EXPECT_TRUE(tab->IsPinned());
  EXPECT_EQ(3, tab_strip_model()->count());
}

// The source tab is the split's pivot, so the Peek page joins its group.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenInSplitViewFromGroupedTab) {
  content::WebContents* source = active_contents();
  AddTab();
  tab_strip_model()->ActivateTabAt(0);
  const tab_groups::TabGroupId group = tab_strip_model()->AddToNewGroup({0});
  OpenPeek(GetURL("b.com", "/title2.html"));
  content::WebContents* contents = peek_contents();

  controller()->OpenInSplitView();
  EXPECT_EQ(contents, active_contents());
  tabs::TabInterface* tab = tab_strip_model()->GetActiveTab();
  ASSERT_TRUE(tab->IsSplit());
  EXPECT_EQ(tab->GetSplit(),
            tabs::TabInterface::GetFromContents(source)->GetSplit());
  EXPECT_EQ(group, tab->GetGroup());
  EXPECT_EQ(3, tab_strip_model()->count());
}

// Joining the source tab's split would replace one of its tabs.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpenInSplitViewFromSplitOpensTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GetURL("a.com", "/title1.html"),
      WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  tab_strip_model()->ActivateTabAt(0);
  tab_strip_model()->AddToNewSplit(
      {1}, split_tabs::SplitTabVisualData(),
      split_tabs::SplitTabCreatedSource::kTabContextMenu);
  OpenPeek(GetURL("b.com", "/title2.html"));
  EXPECT_FALSE(controller()
                   ->peek_view_for_testing()
                   ->split_view_button_for_testing()
                   ->GetVisible());

  content::WebContents* contents = peek_contents();
  controller()->OpenInSplitView();
  EXPECT_EQ(contents, active_contents());
  EXPECT_EQ(3, tab_strip_model()->count());
  EXPECT_FALSE(tab_strip_model()->GetActiveTab()->IsSplit());
}

// A Peek belongs to its tab: switching away hides it, and switching back shows
// the same page again.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, SwitchingTabsHidesAndRestoresPeek) {
  AddTab();
  const GURL url = GetURL("b.com", "/title2.html");
  OpenPeek(url);
  content::WebContents* contents = peek_contents();
  views::Widget* widget = controller()->widget_for_testing();
  ASSERT_TRUE(widget->IsVisible());

  tab_strip_model()->ActivateTabAt(0);
  EXPECT_FALSE(controller()->IsShowing());
  EXPECT_FALSE(widget->IsVisible());
  EXPECT_EQ(contents, controller()->GetWebContentsForTesting(
                          *tab_strip_model()->GetTabAtIndex(1)));
  ASSERT_TRUE(base::test::RunUntil([&] {
    return contents->GetVisibility() == content::Visibility::HIDDEN;
  }));

  tab_strip_model()->ActivateTabAt(1);
  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(contents, peek_contents());
  EXPECT_EQ(widget, controller()->widget_for_testing());
  EXPECT_TRUE(widget->IsVisible());
  EXPECT_EQ(url, contents->GetLastCommittedURL());
  EXPECT_EQ(2, tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, EachTabKeepsItsOwnPeek) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  content::WebContents* first = peek_contents();

  AddTab();
  OpenPeek(GetURL("c.com", "/title2.html"));
  content::WebContents* second = peek_contents();
  ASSERT_NE(first, second);

  tab_strip_model()->ActivateTabAt(0);
  EXPECT_EQ(first, peek_contents());
  EXPECT_EQ(second, controller()->GetWebContentsForTesting(
                        *tab_strip_model()->GetTabAtIndex(1)));

  tab_strip_model()->ActivateTabAt(1);
  EXPECT_EQ(second, peek_contents());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ClosingSourceTabClosesHiddenPeek) {
  AddTab();
  OpenPeek(GetURL("b.com", "/title2.html"));
  content::WebContentsDestroyedWatcher destroyed(peek_contents());
  tab_strip_model()->ActivateTabAt(0);

  tab_strip_model()->CloseWebContentsAt(1, TabCloseTypes::CLOSE_NONE);
  destroyed.Wait();
  EXPECT_EQ(1, tab_strip_model()->count());
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpeningAnotherPeekReplacesIt) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  const GURL second_url = GetURL("c.com", "/title2.html");
  OpenPeek(second_url);
  EXPECT_EQ(second_url, peek_contents()->GetLastCommittedURL());
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

// The title is page-controlled and the host is the real origin, so a long
// title shrinks before the host does.
IN_PROC_BROWSER_TEST_F(PeekBrowserTest, LongTitleKeepsHostVisible) {
  OpenPeek(GetURL("b.com", "/title2.html"));
  const std::u16string long_title(1000, u'x');
  ASSERT_TRUE(content::ExecJs(
      peek_contents(),
      content::JsReplace("document.title = $1;", std::string(1000, 'x'))));

  PeekView* peek_view = controller()->peek_view_for_testing();
  views::Label* title = peek_view->title_for_testing();
  views::Label* host = peek_view->host_for_testing();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return title->GetText() == long_title; }));
  views::test::RunScheduledLayout(peek_view);

  EXPECT_EQ(host->GetPreferredSize().width(), host->width());
  EXPECT_GT(title->GetPreferredSize().width(), title->width());
  EXPECT_TRUE(peek_view->close_button_for_testing()->IsDrawn());
  EXPECT_GT(peek_view->close_button_for_testing()->width(), 0);
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

  // An origin without the initiator's navigation state has no record of the
  // initiator's policies.
  content::OpenURLParams no_initiator_state =
      LinkParams(GetURL("b.com", "/title2.html"));
  no_initiator_state.initiator_navigation_state = nullptr;
  EXPECT_FALSE(controller()->Open(*active_contents(), no_initiator_state));

  EXPECT_FALSE(controller()->Open(*active_contents(),
                                  LinkParams(GURL("data:text/html,hi"))));
  EXPECT_FALSE(controller()->Open(*active_contents(),
                                  LinkParams(GURL("chrome://version"))));
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, NoNestedPeek) {
  OpenPeek(GetURL("b.com", "/title1.html"));
  content::WebContents* contents = peek_contents();
  EXPECT_FALSE(controller()->CanOpenFrom(contents));
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
  controller()->OpenAsTab(*contents);
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

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, OpensFromSplitViewAsStandaloneTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GetURL("a.com", "/title1.html"),
      WindowOpenDisposition::kNewForegroundTab,
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
  controller()->OpenAsTab(*contents);
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
  content::TestNavigationObserver observer(url);
  observer.StartWatchingNewWebContents();
  ASSERT_TRUE(incognito_controller->Open(
      *source, LinkParamsFrom(*source->GetPrimaryMainFrame(), url)));
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
  params.disposition = WindowOpenDisposition::kNewForegroundTab;
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

  // The default partition's cookies must not be sent from the container.
  const std::string path = "/title2.html?container";
  const GURL url = https_server_.GetURL("b.test", path);
  ASSERT_TRUE(content::SetCookie(browser()->GetProfile(), url, "default=1"));

  OpenPeek(url);
  EXPECT_EQ(source_config, peek_contents()
                               ->GetPrimaryMainFrame()
                               ->GetStoragePartition()
                               ->GetConfig());
  std::optional<net::test_server::HttpRequest::HeaderMap> headers;
  ASSERT_TRUE(base::test::RunUntil([&] {
    headers = GetRequestHeaders(path);
    return headers.has_value();
  }));
  EXPECT_EQ(std::string::npos, GetHeader(*headers, "Cookie").find("default=1"));
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
                       LinkNavigatesWithoutPeek) {
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
}

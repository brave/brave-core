/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/peek/peek_features.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "brave/browser/ui/views/peek/peek_controller_impl.h"
#include "brave/browser/ui/views/peek/peek_shields_button.h"
#include "brave/browser/ui/views/peek/peek_view.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view_base.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
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
#include "ui/views/test/button_test_api.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

class PeekBrowserTest : public InProcessBrowserTest {
 public:
  PeekBrowserTest() {
    scoped_features_.InitAndEnableFeature(peek::features::kBravePeek);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
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

  // ExecJs supplies a user gesture, so this follows the path of a real click.
  void ClickLinkTo(content::WebContents* contents,
                   const GURL& url,
                   bool target_blank = false) {
    constexpr char kScript[] = R"(
      const a = document.createElement('a');
      a.href = $1;
      if ($2) {
        a.target = '_blank';
      }
      a.textContent = 'link';
      document.body.appendChild(a);
      a.click();
    )";
    ASSERT_TRUE(content::ExecJs(
        contents, content::JsReplace(kScript, url, target_blank)));
  }

  // Opens `url` in Peek and waits for it to load.
  void OpenPeek(const GURL& url) {
    content::TestNavigationObserver observer(url);
    observer.StartWatchingNewWebContents();
    controller()->Open(content::OpenURLParams(
        url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui::PAGE_TRANSITION_LINK, /*is_renderer_initiated=*/false));
    observer.Wait();
    ASSERT_TRUE(controller()->IsShowing());
  }

  void WaitForPeekClosed() {
    ASSERT_TRUE(
        base::test::RunUntil([&] { return !controller()->IsShowing(); }));
  }

  void PinActiveTab() {
    tab_strip_model()->SetTabPinned(tab_strip_model()->active_index(), true);
  }

  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PinnedTabCrossSiteLinkOpensPeek) {
  const GURL source_url = GetURL("a.com", "/title1.html");
  const GURL target_url = GetURL("b.com", "/title2.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), source_url));
  PinActiveTab();
  const int tab_count = tab_strip_model()->count();

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  ClickLinkTo(active_contents(), target_url);
  observer.Wait();

  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(target_url, controller()->GetWebContents()->GetLastCommittedURL());
  EXPECT_EQ(source_url, active_contents()->GetLastCommittedURL());
  EXPECT_EQ(tab_count, tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PinnedTabTargetBlankLinkOpensPeek) {
  const GURL target_url = GetURL("b.com", "/title2.html");
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/title1.html")));
  PinActiveTab();
  const int tab_count = tab_strip_model()->count();

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  ClickLinkTo(active_contents(), target_url, /*target_blank=*/true);
  observer.Wait();

  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(target_url, controller()->GetWebContents()->GetLastCommittedURL());
  EXPECT_EQ(tab_count, tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, PinnedTabSameSiteLinkNavigates) {
  const GURL target_url = GetURL("sub.a.com", "/title2.html");
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/title1.html")));
  PinActiveTab();

  content::TestNavigationObserver observer(active_contents());
  ClickLinkTo(active_contents(), target_url);
  observer.Wait();

  EXPECT_EQ(target_url, active_contents()->GetLastCommittedURL());
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, UnpinnedTabCrossSiteLinkNavigates) {
  const GURL target_url = GetURL("b.com", "/title2.html");
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/title1.html")));

  content::TestNavigationObserver observer(active_contents());
  ClickLinkTo(active_contents(), target_url);
  observer.Wait();

  EXPECT_EQ(target_url, active_contents()->GetLastCommittedURL());
  EXPECT_FALSE(controller()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ShiftClickOpensPeek) {
  const GURL target_url = GetURL("a.com", "/title2.html");
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GetURL("a.com", "/title1.html")));
  content::WebContents* contents = active_contents();
  ASSERT_TRUE(content::ExecJs(contents, content::JsReplace(R"(
        const a = document.createElement('a');
        a.id = 'link';
        a.href = $1;
        a.style = 'position:absolute;left:0;top:0;width:100px;height:100px;' +
                  'display:block';
        document.body.appendChild(a);
      )",
                                                           target_url)));
  content::SimulateEndOfPaintHoldingOnPrimaryMainFrame(contents);

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  content::SimulateMouseClickAt(
      contents, blink::WebInputEvent::kShiftKey,
      blink::WebMouseEvent::Button::kLeft,
      gfx::ToFlooredPoint(
          content::GetCenterCoordinatesOfElementWithId(contents, "link")));
  observer.Wait();

  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(target_url, controller()->GetWebContents()->GetLastCommittedURL());
  EXPECT_EQ(contents, active_contents());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, ContextMenuOpensPeek) {
  const GURL page_url = GetURL("a.com", "/title1.html");
  const GURL target_url = GetURL("b.com", "/title2.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), page_url));

  content::ContextMenuParams params;
  params.page_url = page_url;
  params.frame_url = page_url;
  params.frame_origin = url::Origin::Create(page_url);
  params.link_url = target_url;
  params.unfiltered_link_url = target_url;
  TestRenderViewContextMenu menu(*active_contents()->GetPrimaryMainFrame(),
                                 params);
  menu.Init();
  ASSERT_TRUE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_OPENLINKPEEK));
  EXPECT_TRUE(menu.IsItemEnabled(IDC_CONTENT_CONTEXT_OPENLINKPEEK));

  content::TestNavigationObserver observer(target_url);
  observer.StartWatchingNewWebContents();
  menu.ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKPEEK, 0);
  observer.Wait();

  ASSERT_TRUE(controller()->IsShowing());
  EXPECT_EQ(target_url, controller()->GetWebContents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, EscapeClosesPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  content::SimulateKeyPress(controller()->GetWebContents(), ui::DomKey::ESCAPE,
                            ui::DomCode::ESCAPE, ui::VKEY_ESCAPE,
                            /*control=*/false, /*shift=*/false, /*alt=*/false,
                            /*command=*/false);
  WaitForPeekClosed();
  EXPECT_FALSE(controller()->GetWebContents());
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
  content::WebContents* peek_contents = controller()->GetWebContents();
  const int tab_count = tab_strip_model()->count();

  views::test::ButtonTestApi(
      controller()->peek_view_for_testing()->open_as_tab_button_for_testing())
      .NotifyDefaultMouseClick();
  WaitForPeekClosed();

  EXPECT_EQ(tab_count + 1, tab_strip_model()->count());
  EXPECT_EQ(peek_contents, active_contents());
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
  EXPECT_EQ(second_url, controller()->GetWebContents()->GetLastCommittedURL());
}

IN_PROC_BROWSER_TEST_F(PeekBrowserTest, SiteInfoButtonShowsPageInfoForPeek) {
  OpenPeek(GetURL("b.com", "/title2.html"));

  views::test::ButtonTestApi(
      controller()->peek_view_for_testing()->site_info_button_for_testing())
      .NotifyDefaultMouseClick();

  EXPECT_EQ(PageInfoBubbleViewBase::BUBBLE_PAGE_INFO,
            PageInfoBubbleViewBase::GetShownBubbleType());
  auto* bubble = static_cast<PageInfoBubbleViewBase*>(
      PageInfoBubbleViewBase::GetPageInfoBubbleForTesting());
  EXPECT_EQ(controller()->GetWebContents(), bubble->web_contents());
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
  controller()->Open(content::OpenURLParams(
      GetURL("b.com", "/title2.html"), content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
      /*is_renderer_initiated=*/false));

  PeekView* peek_view = controller()->peek_view_for_testing();
  gfx::SlideAnimation& animation = peek_view->open_animation_for_testing();
  ASSERT_TRUE(animation.is_animating());
  views::test::RunScheduledLayout(peek_view);
  const int initial_width = peek_view->card_for_testing()->width();

  ASSERT_TRUE(base::test::RunUntil([&] { return !animation.is_animating(); }));
  views::test::RunScheduledLayout(peek_view);
  EXPECT_GT(peek_view->card_for_testing()->width(), initial_width);
}

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

IN_PROC_BROWSER_TEST_F(PeekDisabledBrowserTest, PinnedTabLinkNavigates) {
  EXPECT_FALSE(PeekController::From(browser()));

  const GURL page_url = embedded_test_server()->GetURL("a.com", "/title1.html");
  const GURL target_url =
      embedded_test_server()->GetURL("b.com", "/title2.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), page_url));
  auto* tab_strip_model = browser()->tab_strip_model();
  tab_strip_model->SetTabPinned(0, true);
  content::WebContents* contents = tab_strip_model->GetActiveWebContents();

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

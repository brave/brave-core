/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

class FocusModeTitleBarViewBrowserTest : public InProcessBrowserTest {
 public:
  FocusModeTitleBarViewBrowserTest() = default;
  ~FocusModeTitleBarViewBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());

    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params(
        views::Widget::InitParams::CLIENT_OWNS_WIDGET,
        views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
    params.context = browser()->window()->GetNativeWindow();
    widget_->Init(std::move(params));
    view_ = widget_->SetContentsView(std::make_unique<FocusModeTitleBarView>());
  }

  void TearDownOnMainThread() override {
    view_ = nullptr;
    widget_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  tabs::TabInterface* active_tab() {
    return browser()->tab_strip_model()->GetActiveTab();
  }

  raw_ptr<FocusModeTitleBarView> view_ = nullptr;
  std::unique_ptr<views::Widget> widget_;
};

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest, ReflectsActiveTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("www.example.com", "/title1.html")));
  view_->SetTab(active_tab());
  EXPECT_TRUE(view_->GetDomainTextForTesting().contains(u"example.com"));
  EXPECT_EQ(gfx::ELIDE_HEAD, view_->GetElideBehaviorForTesting());
}

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest, UpdatesOnNavigation) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("www.example.com", "/title1.html")));
  view_->SetTab(active_tab());
  ASSERT_TRUE(view_->GetDomainTextForTesting().contains(u"example.com"));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("docs.example.com", "/title1.html")));

  ASSERT_TRUE(base::test::RunUntil([&]() {
    return view_->GetDomainTextForTesting().contains(u"docs.example.com");
  }));
}

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest, SwitchesBetweenTabs) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("first.test", "/title1.html")));
  view_->SetTab(active_tab());
  ASSERT_TRUE(view_->GetDomainTextForTesting().contains(u"first.test"));

  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), embedded_test_server()->GetURL("second.test", "/title1.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB));
  view_->SetTab(active_tab());
  EXPECT_TRUE(view_->GetDomainTextForTesting().contains(u"second.test"));
}

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest,
                       ChromeSchemeRewrittenToBrave) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings")));
  view_->SetTab(active_tab());
  EXPECT_TRUE(view_->GetDomainTextForTesting().starts_with(u"brave://"));
}

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest, ClearsForNullTab) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("www.example.com", "/title1.html")));
  view_->SetTab(active_tab());
  ASSERT_FALSE(view_->GetDomainTextForTesting().empty());

  view_->SetTab(nullptr);
  EXPECT_TRUE(view_->GetDomainTextForTesting().empty());
  EXPECT_FALSE(view_->IsFaviconVisibleForTesting());
}

IN_PROC_BROWSER_TEST_F(FocusModeTitleBarViewBrowserTest, ClearsOnTabClose) {
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      embedded_test_server()->GetURL("www.example.com", "/title1.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB));
  view_->SetTab(active_tab());
  ASSERT_FALSE(view_->GetDomainTextForTesting().empty());

  browser()->tab_strip_model()->CloseWebContentsAt(
      browser()->tab_strip_model()->active_index(), TabCloseTypes::CLOSE_NONE);

  EXPECT_TRUE(view_->GetDomainTextForTesting().empty());
  EXPECT_FALSE(view_->IsFaviconVisibleForTesting());
}

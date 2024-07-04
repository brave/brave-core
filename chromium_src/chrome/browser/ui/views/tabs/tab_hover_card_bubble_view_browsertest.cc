/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_renderer_data.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_test_util.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

namespace {

constexpr char16_t kTabDomain[] = u"brave://settings";
constexpr char16_t kTabTitle[] = u"Settings - Addresses and more";
constexpr char kTabUrl[] = "chrome://settings/addresses";

}  // namespace

class TabHoverCardBubbleViewBrowserTest : public DialogBrowserTest,
                                          public test::TabHoverCardTestUtil {
 public:
  TabHoverCardBubbleViewBrowserTest() = default;

  TabHoverCardBubbleViewBrowserTest(const TabHoverCardBubbleViewBrowserTest&) =
      delete;
  TabHoverCardBubbleViewBrowserTest& operator=(
      const TabHoverCardBubbleViewBrowserTest&) = delete;

  ~TabHoverCardBubbleViewBrowserTest() override = default;

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    TabRendererData new_tab_data = TabRendererData();
    new_tab_data.title = kTabTitle;
    new_tab_data.last_committed_url = GURL(kTabUrl);
    GetTabStrip(browser())->AddTabAt(0, new_tab_data);

    SimulateHoverTab(browser(), 0);
  }

  bool VerifyUi() override {
    if (!DialogBrowserTest::VerifyUi())
      return false;

    TabStrip* const tab_strip = GetTabStrip(browser());
    Tab* const tab = tab_strip->tab_at(1);
    if (!tab) {
      NOTREACHED_IN_MIGRATION();
      return false;
    }

    TabHoverCardBubbleView* const hover_card = GetHoverCard(tab_strip);
    if (!hover_card) {
      NOTREACHED_IN_MIGRATION();
      return false;
    }

    EXPECT_EQ(kTabTitle, hover_card->GetTitleTextForTesting());
    EXPECT_EQ(kTabDomain, hover_card->GetDomainTextForTesting());
    return true;
  }
};

// This test times out on Windows CI, but works fine locally.
#if BUILDFLAG(IS_WIN)
#define MAYBE_ChromeSchemeUrl DISABLED_ChromeSchemeUrl
#else
#define MAYBE_ChromeSchemeUrl ChromeSchemeUrl
#endif
IN_PROC_BROWSER_TEST_F(TabHoverCardBubbleViewBrowserTest,
                       MAYBE_ChromeSchemeUrl) {
  ShowAndVerifyUi();
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_features.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_types.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/page_state/page_state.h"
#include "third_party/blink/public/common/page_state/page_state_serialization.h"

using BraveSessionRestoreBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveSessionRestoreBrowserTest,
                       SerializationClearNonEmptyPageState) {
  auto* tab_model = browser()->tab_strip_model();
  auto* web_contents = tab_model->GetActiveWebContents();
  SessionService* const session_service =
      SessionServiceFactory::GetForProfile(browser()->profile());
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), GURL("brave://newtab/"), 1);
  ASSERT_EQ(true, EvalJs(web_contents,
                         R"(
        var textarea = document.createElement('textarea')
        textarea.textContent = '__some_text__'
        document.body.append(textarea);
        var input = document.createElement('input')
        input.autocomplete = 'on'
        input.value = '__some_text__'
        document.body.append(input);
        var controls_ready = document.getElementsByTagName('textarea')[0].textContent === '__some_text__' &&
                             document.getElementsByTagName('input')[0].value === '__some_text__';
        controls_ready;
      )"));
  session_service->MoveCurrentSessionToLastSession();
  base::RunLoop loop;
  session_service->GetLastSession(base::BindLambdaForTesting(
      [&](std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
          SessionID ignored_active_window, bool error_reading) {
        EXPECT_EQ(windows.size(), 1u);
        EXPECT_EQ(windows[0]->tabs.size(), 1u);
        EXPECT_EQ(windows[0]->tabs[0]->navigations.size(), 2u);
        const auto& serialized_navigation = windows[0]->tabs[0]->navigations[1];
        EXPECT_EQ(serialized_navigation.virtual_url(),
                  GURL("chrome://newtab/"));

        // Check encoded data is not empty but clean state only with url info.
        EXPECT_EQ(blink::PageState::CreateFromURL(GURL("chrome://newtab/"))
                      .ToEncodedData(),
                  serialized_navigation.encoded_page_state());
        EXPECT_FALSE(serialized_navigation.encoded_page_state().empty());
        loop.Quit();
      }));
  loop.Run();
}

IN_PROC_BROWSER_TEST_F(BraveSessionRestoreBrowserTest,
                       SerializationClearEmptyPageState) {
  auto* tab_model = browser()->tab_strip_model();
  auto* web_contents = tab_model->GetActiveWebContents();
  SessionService* const session_service =
      SessionServiceFactory::GetForProfile(browser()->profile());
  ui_test_utils::NavigateToURLBlockUntilNavigationsComplete(
      browser(), GURL("brave://rewards/"), 1);
  ASSERT_EQ(true, EvalJs(web_contents,
                         R"(
        var textarea = document.createElement('textarea')
        textarea.textContent = '__some_text__'
        document.body.append(textarea);
        var input = document.createElement('input')
        input.autocomplete = 'on'
        input.value = '__some_text__'
        document.body.append(input);
        var controls_ready = document.getElementsByTagName('textarea')[0].textContent === '__some_text__' &&
                             document.getElementsByTagName('input')[0].value === '__some_text__';
        controls_ready;
      )"));
  session_service->MoveCurrentSessionToLastSession();
  base::RunLoop loop;
  session_service->GetLastSession(base::BindLambdaForTesting(
      [&](std::vector<std::unique_ptr<sessions::SessionWindow>> windows,
          SessionID ignored_active_window, bool error_reading) {
        EXPECT_EQ(windows.size(), 1u);
        EXPECT_EQ(windows[0]->tabs.size(), 1u);
        EXPECT_EQ(windows[0]->tabs[0]->navigations.size(), 2u);
        const auto& serialized_navigation = windows[0]->tabs[0]->navigations[1];
        EXPECT_EQ(serialized_navigation.virtual_url(),
                  GURL("chrome://rewards/"));

        // Check encoded data is empty.
        EXPECT_TRUE(serialized_navigation.encoded_page_state().empty());
        loop.Quit();
      }));
  loop.Run();
}

class SessionCookiesCleanupOnSessionRestoreBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  SessionCookiesCleanupOnSessionRestoreBrowserTest() {
    if (ShouldCleanupSessionCookies()) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kBraveCleanupSessionCookiesOnSessionRestore);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kBraveCleanupSessionCookiesOnSessionRestore);
    }
  }

  bool ShouldCleanupSessionCookies() const { return GetParam(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    ASSERT_TRUE(embedded_test_server()->Start());
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        base::StringPrintf("MAP *:80 127.0.0.1:%d",
                           embedded_test_server()->port()));
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_P(SessionCookiesCleanupOnSessionRestoreBrowserTest,
                       PRE_CleanupSessionCookies) {
  auto* rfh =
      ui_test_utils::NavigateToURL(browser(), GURL("http://a.test/empty.html"));
  ASSERT_TRUE(rfh);
  ASSERT_TRUE(content::ExecJs(rfh, "document.cookie = 'bar=session'"));
}

IN_PROC_BROWSER_TEST_P(SessionCookiesCleanupOnSessionRestoreBrowserTest,
                       CleanupSessionCookies) {
  auto* rfh =
      ui_test_utils::NavigateToURL(browser(), GURL("http://a.test/empty.html"));
  ASSERT_TRUE(rfh);
  if (ShouldCleanupSessionCookies()) {
    EXPECT_EQ("", content::EvalJs(rfh, "document.cookie"));
  } else {
    EXPECT_EQ("bar=session", content::EvalJs(rfh, "document.cookie"));
  }
}

INSTANTIATE_TEST_SUITE_P(,
                         SessionCookiesCleanupOnSessionRestoreBrowserTest,
                         testing::Bool());

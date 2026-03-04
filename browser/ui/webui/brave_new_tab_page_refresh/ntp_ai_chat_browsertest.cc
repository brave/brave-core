/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Helper to wait for an element to exist (or not exist) in the DOM using
// JavaScript MutationObserver. This avoids nested run loop issues.
bool WaitForElement(content::WebContents* web_contents,
                    const std::string& selector,
                    bool should_exist = true) {
  constexpr char kWaitForElementScript[] = R"(
    new Promise((resolve) => {
      const selector = '[data-test-id="' + $1 + '"]';
      const shouldExist = $2;
      const timeout = 10000;

      function checkElement() {
        const element = document.querySelector(selector);
        if (shouldExist && element) {
          return true;
        }
        if (!shouldExist && !element) {
          return true;
        }
        return false;
      }

      if (checkElement()) {
        resolve(true);
        return;
      }

      const observer = new MutationObserver(() => {
        if (checkElement()) {
          observer.disconnect();
          resolve(true);
        }
      });
      observer.observe(document.documentElement,
          { childList: true, subtree: true });

      // Timeout after 10 seconds
      setTimeout(() => {
        observer.disconnect();
        resolve(checkElement());
      }, timeout);
    })
  )";

  auto result = content::EvalJs(
      web_contents,
      content::JsReplace(kWaitForElementScript, selector, should_exist));
  return result.ExtractBool();
}

// Helper to click an element by its data-test-id
bool ClickElement(content::WebContents* web_contents,
                  const std::string& test_id) {
  if (!WaitForElement(web_contents, test_id, true)) {
    return false;
  }

  return content::ExecJs(
      web_contents,
      content::JsReplace(
          R"(document.querySelector('[data-test-id="' + $1 + '"]').click())",
          test_id));
}

// Helper to enter text into an editable element
bool EnterText(content::WebContents* web_contents,
               const std::string& test_id,
               const std::string& text) {
  if (!WaitForElement(web_contents, test_id, true)) {
    return false;
  }

  // The AI Chat input is a contenteditable div, so we need to focus and set
  // innerText
  return content::ExecJs(web_contents, content::JsReplace(
                                           R"(
      (function() {
        const el = document.querySelector('[data-test-id="' + $1 + '"]');
        el.focus();
        el.innerText = $2;
        // Dispatch input event to trigger React state update
        el.dispatchEvent(new InputEvent('input', {
          bubbles: true,
          cancelable: true,
          inputType: 'insertText',
          data: $2
        }));
      })()
    )",
                                           test_id, text));
}

// Helper to wait for the submit button to become enabled
bool WaitForSubmitButtonEnabled(content::WebContents* web_contents) {
  constexpr char kWaitForButtonEnabledScript[] = R"(
    new Promise((resolve) => {
      const timeout = 10000;

      function checkButton() {
        const btn = document.querySelector(`[data-test-id='leo-submit-button']`);
        return btn && !btn.disabled;
      }

      if (checkButton()) {
        resolve(true);
        return;
      }

      const observer = new MutationObserver(() => {
        if (checkButton()) {
          observer.disconnect();
          resolve(true);
        }
      });
      observer.observe(document.documentElement,
          { childList: true, subtree: true, attributes: true });

      setTimeout(() => {
        observer.disconnect();
        resolve(checkButton());
      }, timeout);
    })
  )";

  return content::EvalJs(web_contents, kWaitForButtonEnabledScript)
      .ExtractBool();
}

}  // namespace

class NTPAIChatBrowserTest : public InProcessBrowserTest {
 public:
  NTPAIChatBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {ai_chat::features::kAIChat,
         ai_chat::features::kShowAIChatInputOnNewTabPage},
        {});
  }

  ~NTPAIChatBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Enable the show chat input pref
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_search_conversion::prefs::kShowNTPChatInput, true);
  }

 protected:
  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void NavigateToNewTab(content::WebContents* web_contents) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                             GURL(chrome::kChromeUINewTabURL)));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  }

  bool VerifyNewTabPageLoaded(content::WebContents* web_contents) {
    return content::EvalJs(
               web_contents,
               R"(!!document.querySelector(`html[data-test-id='brave-new-tab-page']`))")
        .ExtractBool();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test that the query mode toggle is visible when both search box and chat
// input are enabled.
IN_PROC_BROWSER_TEST_F(NTPAIChatBrowserTest, QueryModeToggleVisible) {
  content::WebContents* web_contents = GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  NavigateToNewTab(web_contents);
  EXPECT_TRUE(VerifyNewTabPageLoaded(web_contents));

  // The toggle should be visible since both search box and chat input are
  // enabled
  EXPECT_TRUE(WaitForElement(web_contents, "query-mode-toggle", true))
      << "Query mode toggle should be visible";

  // Both toggle buttons should be present
  EXPECT_TRUE(WaitForElement(web_contents, "query-mode-toggle-search", true))
      << "Search toggle button should be visible";
  EXPECT_TRUE(WaitForElement(web_contents, "query-mode-toggle-chat", true))
      << "Chat toggle button should be visible";
}

// Test that clicking the chat toggle shows the chat input
IN_PROC_BROWSER_TEST_F(NTPAIChatBrowserTest, SwitchToChatMode) {
  content::WebContents* web_contents = GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  NavigateToNewTab(web_contents);
  EXPECT_TRUE(VerifyNewTabPageLoaded(web_contents));

  // Wait for toggle to be visible
  ASSERT_TRUE(WaitForElement(web_contents, "query-mode-toggle", true));

  // Initially should be in search mode, so search input should be visible
  EXPECT_TRUE(WaitForElement(web_contents, "ntp-search-input", true))
      << "Search input should be visible by default";

  // Click on the chat toggle button to switch to chat mode
  ASSERT_TRUE(ClickElement(web_contents, "query-mode-toggle-chat"));

  // Now the chat input should be visible
  EXPECT_TRUE(WaitForElement(web_contents, "ntp-chat-input", true))
      << "Chat input should be visible after clicking chat toggle";

  // And the search input should no longer be visible
  EXPECT_TRUE(WaitForElement(web_contents, "ntp-search-input", false))
      << "Search input should not be visible in chat mode";
}

// Test that submitting text in the chat input creates a conversation and
// navigates to the conversation URL.
IN_PROC_BROWSER_TEST_F(NTPAIChatBrowserTest, SubmitChatMessage) {
  content::WebContents* web_contents = GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  NavigateToNewTab(web_contents);
  EXPECT_TRUE(VerifyNewTabPageLoaded(web_contents));

  // Switch to chat mode
  ASSERT_TRUE(WaitForElement(web_contents, "query-mode-toggle-chat", true));
  ASSERT_TRUE(ClickElement(web_contents, "query-mode-toggle-chat"));

  // Wait for chat input to be visible
  ASSERT_TRUE(WaitForElement(web_contents, "ntp-chat-input", true));

  // Enter text into the chat input
  ASSERT_TRUE(
      EnterText(web_contents, "leo-input", "Hello, this is a test message"));

  // Wait for the submit button to become enabled
  ASSERT_TRUE(WaitForSubmitButtonEnabled(web_contents))
      << "Submit button should become enabled after entering text";

  // Set up a navigation observer before clicking submit
  content::TestNavigationObserver nav_observer(web_contents);

  // Click the submit button
  ASSERT_TRUE(ClickElement(web_contents, "leo-submit-button"));

  // Wait for navigation to complete
  nav_observer.Wait();

  // Verify the URL contains a conversation UUID
  GURL final_url = web_contents->GetLastCommittedURL();
  EXPECT_TRUE(final_url.SchemeIs("chrome"));
  EXPECT_EQ(final_url.host(), "leo-ai");
  // The path should contain a UUID (non-empty path after the leading slash)
  EXPECT_FALSE(final_url.path().empty());
  EXPECT_NE(final_url.path(), "/");
}

// Test that when the feature flag is disabled, the chat toggle is not visible
class NTPAIChatDisabledBrowserTest : public InProcessBrowserTest {
 public:
  NTPAIChatDisabledBrowserTest() {
    // Enable AI Chat but disable the NTP input feature
    scoped_feature_list_.InitWithFeatures(
        {ai_chat::features::kAIChat},
        {ai_chat::features::kShowAIChatInputOnNewTabPage});
  }

  ~NTPAIChatDisabledBrowserTest() override = default;

 protected:
  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void NavigateToNewTab(content::WebContents* web_contents) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                             GURL(chrome::kChromeUINewTabURL)));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(NTPAIChatDisabledBrowserTest,
                       ChatToggleNotVisibleWhenFeatureDisabled) {
  content::WebContents* web_contents = GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  NavigateToNewTab(web_contents);

  // Wait for the page to load
  ASSERT_TRUE(
      content::EvalJs(
          web_contents,
          R"(!!document.querySelector(`html[data-test-id='brave-new-tab-page']`))")
          .ExtractBool());

  // The chat toggle should not be visible since the feature is disabled
  EXPECT_TRUE(WaitForElement(web_contents, "query-mode-toggle", false))
      << "Query mode toggle should not be visible when feature is disabled";
}

// Test that when the pref is disabled, only the search input shows without
// toggle
class NTPAIChatPrefDisabledBrowserTest : public InProcessBrowserTest {
 public:
  NTPAIChatPrefDisabledBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {ai_chat::features::kAIChat,
         ai_chat::features::kShowAIChatInputOnNewTabPage},
        {});
  }

  ~NTPAIChatPrefDisabledBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Disable the show chat input pref
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_search_conversion::prefs::kShowNTPChatInput, false);
  }

 protected:
  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void NavigateToNewTab(content::WebContents* web_contents) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(),
                                             GURL(chrome::kChromeUINewTabURL)));
    ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(NTPAIChatPrefDisabledBrowserTest,
                       ToggleNotVisibleWhenPrefDisabled) {
  content::WebContents* web_contents = GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  NavigateToNewTab(web_contents);

  // Wait for the page to load
  ASSERT_TRUE(
      content::EvalJs(
          web_contents,
          R"(!!document.querySelector(`html[data-test-id='brave-new-tab-page']`))")
          .ExtractBool());

  // The toggle should not be visible since the pref is disabled
  EXPECT_TRUE(WaitForElement(web_contents, "query-mode-toggle", false))
      << "Query mode toggle should not be visible when pref is disabled";

  // But the search input should still be visible
  EXPECT_TRUE(WaitForElement(web_contents, "ntp-search-input", true))
      << "Search input should still be visible";
}

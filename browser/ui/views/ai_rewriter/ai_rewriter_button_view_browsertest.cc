// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/ai_rewriter/ai_rewriter_button_view.h"

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "brave/browser/ai_rewriter/ai_rewriter_tab_helper.h"
#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace ai_rewriter {

class AIRewriterButtonViewBrowserTest : public InProcessBrowserTest {
 public:
  AIRewriterButtonViewBrowserTest() {
    features_.InitAndEnableFeature(features::kAIRewriter);
  }

  AIRewriterButtonViewBrowserTest(const AIRewriterButtonViewBrowserTest&) =
      delete;
  AIRewriterButtonViewBrowserTest& operator=(
      const AIRewriterButtonViewBrowserTest&) = delete;
  ~AIRewriterButtonViewBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    auto test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  GURL GetURL(std::string path) {
    return embedded_test_server()->GetURL("example.com", path);
  }

  void NavigateToPath(std::string path) {
    auto url = GetURL(path);

    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    content::WaitForLoadStop(contents());
  }

  void OpenPageWithInputAndSelectTextInSelector(std::string selector) {
    NavigateToPath("/rewriter-example.html");
    SelectTextInSelectorOrClear(selector);
  }

  void SelectTextInSelectorOrClear(std::string selector) {
    EXPECT_TRUE(
        content::ExecJs(contents()->GetPrimaryMainFrame(),
                        base::StrCat({"selectText('", selector, "')"})));
  }

  void WaitForVisibilityChange() {
    auto* helper = AIRewriterTabHelper::FromWebContents(contents());
    CHECK(helper);

    base::RunLoop loop;
    helper->SetOnVisibilityChangeForTesting(loop.QuitClosure());
    loop.Run();
  }

 private:
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonDoesNotShowWhenNoTextSelected) {
  OpenPageWithInputAndSelectTextInSelector("");

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);
  EXPECT_FALSE(tab_helper->button_for_testing());
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonDoesNotShowWhenNonEditableTextSelected) {
  OpenPageWithInputAndSelectTextInSelector("#non-editable");

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);
  EXPECT_FALSE(tab_helper->button_for_testing());
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonShowsWhenEditableTextSelected) {
  OpenPageWithInputAndSelectTextInSelector("#editable");
  WaitForVisibilityChange();

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->button_for_testing());
  EXPECT_TRUE(tab_helper->button_for_testing()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ShownButtonHidesWhenNonEditableIsSelected) {
  OpenPageWithInputAndSelectTextInSelector("#editable");
  WaitForVisibilityChange();

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->button_for_testing());
  EXPECT_TRUE(tab_helper->button_for_testing()->IsShowing());

  SelectTextInSelectorOrClear("#non-editable");
  WaitForVisibilityChange();

  EXPECT_TRUE(tab_helper->button_for_testing());
  EXPECT_FALSE(tab_helper->button_for_testing()->IsShowing());
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonClosedWhenPageNavigated) {
  OpenPageWithInputAndSelectTextInSelector("#editable");
  WaitForVisibilityChange();

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->button_for_testing());
  EXPECT_TRUE(tab_helper->button_for_testing()->IsShowing());

  NavigateToPath("/simple.html");

  EXPECT_FALSE(tab_helper->button_for_testing());
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonClosedWhenTabReparented) {
  OpenPageWithInputAndSelectTextInSelector("#editable");
  WaitForVisibilityChange();

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);
  EXPECT_TRUE(tab_helper->button_for_testing());
  EXPECT_TRUE(tab_helper->button_for_testing()->IsShowing());

  NavigateToPath("/simple.html");

  EXPECT_FALSE(tab_helper->button_for_testing());
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonDestroyedWhenTabClosed) {
  OpenPageWithInputAndSelectTextInSelector("#editable");
  WaitForVisibilityChange();

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);

  auto button = tab_helper->button_for_testing();
  EXPECT_TRUE(button);
  EXPECT_TRUE(button->IsShowing());

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GetURL("/simple.html"),
      WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  browser()->tab_strip_model()->CloseWebContentsAt(
      0, TabCloseTypes::CLOSE_USER_GESTURE);

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(button);
}

IN_PROC_BROWSER_TEST_F(AIRewriterButtonViewBrowserTest,
                       ButtonClickOpensRewriterDialog) {
  OpenPageWithInputAndSelectTextInSelector("#editable");
  WaitForVisibilityChange();

  auto* tab_helper = AIRewriterTabHelper::FromWebContents(contents());
  EXPECT_TRUE(tab_helper);

  auto button = tab_helper->button_for_testing();
  EXPECT_TRUE(button);

  auto* dialog = static_cast<AIRewriterButtonView*>(button.get())->OpenDialog();
  EXPECT_TRUE(dialog);
}

}  // namespace ai_rewriter

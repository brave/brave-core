// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_rewriter/ai_rewriter_dialog_delegate.h"

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

class AIRewriterDialogDelegateBrowserTest : public InProcessBrowserTest {
 public:
  AIRewriterDialogDelegateBrowserTest() {
    features_.InitAndEnableFeature(ai_rewriter::features::kAIRewriter);
  }

  AIRewriterDialogDelegateBrowserTest(
      const AIRewriterDialogDelegateBrowserTest&) = delete;
  AIRewriterDialogDelegateBrowserTest& operator=(
      const AIRewriterDialogDelegateBrowserTest&) = delete;
  ~AIRewriterDialogDelegateBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    auto test_data_dir = base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    html_with_input_ =
        embedded_test_server()->GetURL("example.com", "/rewriter-example.html");
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  views::Widget* browser_widget() {
    return views::Widget::GetWidgetForNativeWindow(
        browser()->window()->GetNativeWindow());
  }

  void MoveWindow(gfx::Vector2d by) {
    auto bounds = browser_widget()->GetWindowBoundsInScreen();
    bounds.Offset(by);
    browser_widget()->SetBounds(bounds);
  }

  void OpenPageWithInput() {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), html_with_input_));
    content::WaitForLoadStop(contents());

    EXPECT_TRUE(content::ExecJs(contents()->GetFocusedFrame(),
                                "document.querySelector('textarea').select()"));
  }

 private:
  base::test::ScopedFeatureList features_;
  GURL html_with_input_;
};

IN_PROC_BROWSER_TEST_F(AIRewriterDialogDelegateBrowserTest, CanShowADialog) {
  OpenPageWithInput();

  ASSERT_TRUE(ai_rewriter::AIRewriterDialogDelegate::Show(
      contents(), "It was a dark and stormy night..."));
}

// For now, Dialog positioning doesn't work on MacOS.
#if !BUILDFLAG(IS_MAC)
#define MAYBE_DialogMovesWithParentWindow DialogMovesWithParentWindow
#else
#define MAYBE_DialogMovesWithParentWindow DISABLED_DialogMovesWithParentWindow
#endif
IN_PROC_BROWSER_TEST_F(AIRewriterDialogDelegateBrowserTest,
                       MAYBE_DialogMovesWithParentWindow) {
  OpenPageWithInput();

  auto* dialog = ai_rewriter::AIRewriterDialogDelegate::Show(
      contents(), "It was a dark and stormy night...");

  auto* dialog_widget = dialog->widget_for_testing();
  EXPECT_TRUE(dialog_widget);

  auto position = dialog_widget->GetWindowBoundsInScreen().origin();

  gfx::Vector2d offset(100, 100);
  MoveWindow(offset);

  auto next_position = dialog_widget->GetWindowBoundsInScreen().origin();
  EXPECT_EQ(offset, next_position - position);
}

IN_PROC_BROWSER_TEST_F(AIRewriterDialogDelegateBrowserTest,
                       ChangingFocusClosesDialog) {
  OpenPageWithInput();

  auto* dialog = ai_rewriter::AIRewriterDialogDelegate::Show(
      contents(), "It was a dark and stormy night...");

  auto dialog_widget = dialog->widget_for_testing()->GetWeakPtr();
  EXPECT_TRUE(content::ExecJs(contents()->GetFocusedFrame(),
                              "document.querySelector('textarea').blur()"));
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(dialog_widget);
}

IN_PROC_BROWSER_TEST_F(AIRewriterDialogDelegateBrowserTest, CanInsertText) {
  OpenPageWithInput();

  auto* dialog = ai_rewriter::AIRewriterDialogDelegate::Show(
      contents(), "It was a dark and stormy night...");

  std::string mock_generated("It was a bright and sunny day...");
  dialog->GetRewriterUIForTesting()->InsertTextAndClose(mock_generated,
                                                        base::DoNothing());

  EXPECT_EQ(mock_generated,
            content::EvalJs(contents()->GetFocusedFrame(),
                            R"(document.querySelector('textarea').value)"));
}

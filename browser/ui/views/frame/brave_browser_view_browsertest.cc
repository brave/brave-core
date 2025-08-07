/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view.h"

#include <string>

#include "build/build_config.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/test/test_browser_ui.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/scrim_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {
class TestTabModalConfirmDialogDelegate : public TabModalConfirmDialogDelegate {
 public:
  explicit TestTabModalConfirmDialogDelegate(content::WebContents* contents)
      : TabModalConfirmDialogDelegate(contents) {}

  TestTabModalConfirmDialogDelegate(const TestTabModalConfirmDialogDelegate&) =
      delete;
  TestTabModalConfirmDialogDelegate& operator=(
      const TestTabModalConfirmDialogDelegate&) = delete;

  std::u16string GetTitle() override { return std::u16string(u"Dialog Title"); }
  std::u16string GetDialogMessage() override { return std::u16string(); }
};
}  // namespace

class BraveBrowserViewTest : public InProcessBrowserTest {
 public:
  BraveBrowserViewTest() = default;
  ~BraveBrowserViewTest() override = default;

  BraveBrowserViewTest(const BraveBrowserViewTest&) = delete;
  BraveBrowserViewTest& operator=(const BraveBrowserViewTest&) = delete;

 protected:
  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }
};

// Tests that a content area scrim is still disabled when a tab modal dialog is
// active.
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest, ScrimForTabModalDisabledTest) {
  content::WebContents* contents = browser_view()->GetActiveWebContents();
  auto delegate = std::make_unique<TestTabModalConfirmDialogDelegate>(contents);

  // Check scrim view is always not visible.
  TabModalConfirmDialog::Create(std::move(delegate), contents);
  EXPECT_FALSE(browser_view()
                   ->GetActiveContentsContainerView()
                   ->GetContentsScrimView()
                   ->GetVisible());

  ASSERT_TRUE(
      AddTabAtIndex(1, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK));
  EXPECT_FALSE(browser_view()
                   ->GetActiveContentsContainerView()
                   ->GetContentsScrimView()
                   ->GetVisible());

  browser()->tab_strip_model()->ActivateTabAt(
      0, TabStripUserGestureDetails(
             TabStripUserGestureDetails::GestureType::kMouse));
  EXPECT_FALSE(browser_view()
                   ->GetActiveContentsContainerView()
                   ->GetContentsScrimView()
                   ->GetVisible());
}

// MacOS does not need views window scrim. We use sheet to show window modals
// (-[NSWindow beginSheet:]), which natively draws a scrim since macOS 11.
// Tests that a scrim is still disabled when a window modal dialog is active.
#if !BUILDFLAG(IS_MAC)
IN_PROC_BROWSER_TEST_F(BraveBrowserViewTest,
                       ScrimForBrowserWindowModalDisabledTest) {
  auto child_widget_delegate = std::make_unique<views::WidgetDelegate>();
  auto child_widget = std::make_unique<views::Widget>();
  child_widget_delegate->SetModalType(ui::mojom::ModalType::kWindow);
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = child_widget_delegate.get();
  params.parent = browser_view()->GetWidget()->GetNativeView();
  child_widget->Init(std::move(params));

  // Check scrim view is always not visible.
  child_widget->Show();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
  child_widget->Hide();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
  child_widget->Show();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
  child_widget.reset();
  EXPECT_FALSE(browser_view()->window_scrim_view()->GetVisible());
}
#endif  // !BUILDFLAG(IS_MAC)

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/screenshot_preview_dialog.h"

#include <vector>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/test/test_future.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/views/widget/any_widget_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/dialog_delegate.h"

namespace screenshot {

namespace {

std::vector<uint8_t> MakeTestPng() {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(4, 4);
  bitmap.eraseColor(SK_ColorBLUE);
  auto encoded =
      gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, /*discard_transparency=*/true);
  CHECK(encoded);
  return *encoded;
}

// Waits for the real (views-based) preview dialog to be shown and hands back
// its Widget, identified by its window title.
class PreviewWidgetWaiter {
 public:
  PreviewWidgetWaiter() : observer_(views::test::AnyWidgetTestPasskey()) {
    observer_.set_shown_callback(base::BindRepeating(
        &PreviewWidgetWaiter::OnWidgetShown, base::Unretained(this)));
  }

  views::Widget* Wait() {
    if (!widget_) {
      run_loop_.Run();
    }
    auto* widget = widget_.get();
    // Clears the pointer so that this doesn't reference a dangling Widget after
    // it closes.
    widget_ = nullptr;
    return widget;
  }

 private:
  void OnWidgetShown(views::Widget* widget) {
    if (widget->widget_delegate()->GetWindowTitle() !=
        l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_PREVIEW_DIALOG_TITLE)) {
      return;
    }
    widget_ = widget;
    run_loop_.Quit();
  }

  views::AnyWidgetObserver observer_;
  base::RunLoop run_loop_;
  raw_ptr<views::Widget> widget_ = nullptr;
};

// Blocks until a specific widget is destroyed.
class WidgetDestructionWaiter : public views::WidgetObserver {
 public:
  explicit WidgetDestructionWaiter(views::Widget* widget) {
    observation_.Observe(widget);
  }

  void Wait() { run_loop_.Run(); }

 private:
  void OnWidgetDestroyed(views::Widget* widget) override {
    observation_.Reset();
    run_loop_.Quit();
  }

  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};
  base::RunLoop run_loop_;
};

}  // namespace

using ScreenshotPreviewDialogBrowserTest = InProcessBrowserTest;

// Exercises the real ScreenshotPreviewDialogDelegate/Holder pair (not the fake
// PreviewDialogShower the ScreenshotController unit tests use), covering the
// Download button, Esc, and the close (X) control.
IN_PROC_BROWSER_TEST_F(ScreenshotPreviewDialogBrowserTest,
                       DownloadButton_RunsOnDownloadAndClosesWidget) {
  PreviewWidgetWaiter widget_waiter;
  base::test::TestFuture<std::vector<uint8_t>> on_download;
  base::test::TestFuture<void> on_cancel;

  std::vector<uint8_t> png = MakeTestPng();
  ShowScreenshotPreviewDialog(browser()->GetWindow()->GetNativeWindow(), png,
                              on_download.GetCallback(),
                              on_cancel.GetCallback());

  views::Widget* widget = widget_waiter.Wait();
  ASSERT_TRUE(widget);
  WidgetDestructionWaiter destruction_waiter(widget);

  widget->widget_delegate()->AsDialogDelegate()->AcceptDialog();

  EXPECT_EQ(on_download.Get(), png);
  EXPECT_FALSE(on_cancel.IsReady());
  destruction_waiter.Wait();
}

IN_PROC_BROWSER_TEST_F(ScreenshotPreviewDialogBrowserTest,
                       EscapeKey_RunsOnCancelAndClosesWidget) {
  PreviewWidgetWaiter widget_waiter;
  base::test::TestFuture<std::vector<uint8_t>> on_download;
  base::test::TestFuture<void> on_cancel;

  ShowScreenshotPreviewDialog(browser()->GetWindow()->GetNativeWindow(),
                              MakeTestPng(), on_download.GetCallback(),
                              on_cancel.GetCallback());

  views::Widget* widget = widget_waiter.Wait();
  ASSERT_TRUE(widget);
  WidgetDestructionWaiter destruction_waiter(widget);

  widget->CloseWithReason(views::Widget::ClosedReason::kEscKeyPressed);

  EXPECT_TRUE(on_cancel.Wait());
  EXPECT_FALSE(on_download.IsReady());
  destruction_waiter.Wait();
}

IN_PROC_BROWSER_TEST_F(ScreenshotPreviewDialogBrowserTest,
                       CloseButton_RunsOnCancelAndClosesWidget) {
  PreviewWidgetWaiter widget_waiter;
  base::test::TestFuture<std::vector<uint8_t>> on_download;
  base::test::TestFuture<void> on_cancel;

  ShowScreenshotPreviewDialog(browser()->GetWindow()->GetNativeWindow(),
                              MakeTestPng(), on_download.GetCallback(),
                              on_cancel.GetCallback());

  views::Widget* widget = widget_waiter.Wait();
  ASSERT_TRUE(widget);
  WidgetDestructionWaiter destruction_waiter(widget);

  widget->CloseWithReason(views::Widget::ClosedReason::kCloseButtonClicked);

  EXPECT_TRUE(on_cancel.Wait());
  EXPECT_FALSE(on_download.IsReady());
  destruction_waiter.Wait();
}

}  // namespace screenshot

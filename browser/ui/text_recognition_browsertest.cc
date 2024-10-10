/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/browser/ui/views/text_recognition_dialog_tracker.h"
#include "brave/browser/ui/views/text_recognition_dialog_view.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "text_recognition";

}  // namespace

class TextRecognitionBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
    image_html_url_ = embedded_test_server()->GetURL("a.com", "/image.html");
  }

  void OnGetTextFromImage(
      const std::pair<bool, std::vector<std::string>>& supported_strs) {
    // Test image has "brave" text.
    EXPECT_TRUE(supported_strs.first);
    EXPECT_EQ("brave", supported_strs.second[0]);
    run_loop_->Quit();
  }

  void OnGetImageForTextCopy(base::WeakPtr<content::WebContents> web_contents,
                             const SkBitmap& image) {
    if (!web_contents)
      return;

    brave::ShowTextRecognitionDialog(web_contents.get(), image);
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run())
      return;

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition] {
                      if (condition.Run())
                        run_loop_->Quit();
                    }));
    Run();
  }

  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop()->Run();
  }

  base::RunLoop* run_loop() const { return run_loop_.get(); }

  GURL image_html_url_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(TextRecognitionBrowserTest, TextRecognitionTest) {
  content::ContextMenuParams params;
  params.media_type = blink::mojom::ContextMenuDataMediaType::kImage;

  // kImage type can have copy text from image menu entry.
  {
    TestRenderViewContextMenu menu(*browser()
                                        ->tab_strip_model()
                                        ->GetActiveWebContents()
                                        ->GetPrimaryMainFrame(),
                                   params);
    menu.Init();

    EXPECT_TRUE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE));
  }

  // Other type should not have.
  params.media_type = blink::mojom::ContextMenuDataMediaType::kVideo;
  {
    TestRenderViewContextMenu menu(*browser()
                                        ->tab_strip_model()
                                        ->GetActiveWebContents()
                                        ->GetPrimaryMainFrame(),
                                   params);
    menu.Init();

    EXPECT_FALSE(menu.IsItemPresent(IDC_CONTENT_CONTEXT_COPY_TEXT_FROM_IMAGE));
  }

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), image_html_url_));
  ASSERT_TRUE(WaitForLoadStop(contents));

  // Using (10, 10) position will be fine because test image is set at (0, 0).
  browser()
      ->tab_strip_model()
      ->GetActiveWebContents()
      ->GetPrimaryMainFrame()
      ->GetImageAt(
          10, 10,
          base::BindOnce(&TextRecognitionBrowserTest::OnGetImageForTextCopy,
                         base::Unretained(this), contents->GetWeakPtr()));
  TextRecognitionDialogTracker::CreateForWebContents(contents);
  auto* dialog_tracker =
      TextRecognitionDialogTracker::FromWebContents(contents);

  // Wait till text recognition dialog is launched.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !!dialog_tracker->active_dialog(); }));

  TextRecognitionDialogView* text_recognition_dialog =
      static_cast<TextRecognitionDialogView*>(
          dialog_tracker->active_dialog()->widget_delegate());

  // Early check - extracting could be done very quickly.
  if (text_recognition_dialog->scroll_view_) {
    const auto text = static_cast<views::Label*>(
                          text_recognition_dialog->scroll_view_->contents())
                          ->GetText();
    if (text == u"brave") {
      return;
    }
  }

  // OnGetTextFromImage() verifies extracted text from test image.
  text_recognition_dialog->on_get_text_callback_for_test_ = base::BindOnce(
      &TextRecognitionBrowserTest::OnGetTextFromImage, base::Unretained(this));

  Run();
}

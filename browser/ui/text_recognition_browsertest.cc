/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/browser/ui/views/text_recognition_dialog_tracker.h"
#include "brave/browser/ui/views/text_recognition_dialog_view.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/ui/base/clipboard/test/privacy_capturing_test_clipboard.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "text_recognition";

}  // namespace

class TextRecognitionBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    clipboard_ =
        brave::PrivacyCapturingTestClipboard::InstallForCurrentThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
    image_html_url_ = embedded_test_server()->GetURL("a.com", "/image.html");
  }

  void TearDownOnMainThread() override {
    clipboard_ = nullptr;
    ui::Clipboard::DestroyClipboardForCurrentThread();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void OnGetTextFromImage(
      const std::pair<bool, std::vector<std::string>>& supported_strs) {
    // Test image has "brave" text.
    EXPECT_TRUE(supported_strs.first);
    EXPECT_EQ("brave", supported_strs.second[0]);
    run_loop_->Quit();
  }

  // Runs the "Copy Text From Image" flow on the test image in |target| and
  // returns once the recognized text has been written to the clipboard.
  void RunTextRecognitionFlow(BrowserWindowInterface* target) {
    content::WebContents* contents =
        target->tab_strip_model()->GetActiveWebContents();
    ASSERT_TRUE(ui_test_utils::NavigateToURL(target, image_html_url_));
    ASSERT_TRUE(WaitForLoadStop(contents));

    // Using (10, 10) position will be fine because test image is set at (0, 0).
    contents->GetPrimaryMainFrame()->GetImageAt(
        10, 10,
        base::BindOnce(&TextRecognitionBrowserTest::OnGetImageForTextCopy,
                       base::Unretained(this), contents->GetWeakPtr()));
    TextRecognitionDialogTracker::CreateForWebContents(contents);
    auto* dialog_tracker =
        TextRecognitionDialogTracker::FromWebContents(contents);

    // Wait till text recognition dialog is launched.
    WaitUntil(base::BindLambdaForTesting(
        [&]() { return !!dialog_tracker->active_dialog(); }));

    auto* dialog = views::AsViewClass<TextRecognitionDialogView>(
        dialog_tracker->active_dialog()->widget_delegate()->GetContentsView());
    ASSERT_TRUE(dialog);

    // Early check - extracting could be done very quickly.
    if (dialog->GetDisplayedTextForTesting() == u"brave") {
      return;
    }

    // OnGetTextFromImage() verifies extracted text from test image.
    dialog->SetOnGetTextCallbackForTesting(
        base::BindOnce(&TextRecognitionBrowserTest::OnGetTextFromImage,
                       base::Unretained(this)));
    Run();
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
  raw_ptr<brave::PrivacyCapturingTestClipboard> clipboard_ = nullptr;
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

  ASSERT_NO_FATAL_FAILURE(RunTextRecognitionFlow(browser()));

  // A normal profile copy stays eligible for OS clipboard history and cloud
  // clipboard sync.
  constexpr uint32_t kNoPrivacyTypes = ui::Clipboard::kNone;
  EXPECT_EQ(kNoPrivacyTypes, clipboard_->last_privacy_types());
}

// Text recognized from an image in a private/Tor window must not leak into the
// OS clipboard history or cloud clipboard sync.
IN_PROC_BROWSER_TEST_F(TextRecognitionBrowserTest,
                       OffTheRecordTextIsNotSharedWithOS) {
  ASSERT_NO_FATAL_FAILURE(RunTextRecognitionFlow(CreateIncognitoBrowser()));

  constexpr uint32_t kExpectedPrivacyTypes =
      ui::Clipboard::kNoLocalClipboardHistory |
      ui::Clipboard::kNoCloudClipboard;
  EXPECT_EQ(kExpectedPrivacyTypes, clipboard_->last_privacy_types());
}

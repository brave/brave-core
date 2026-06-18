// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshot/screenshot_controller.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "printing/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/native_ui_types.h"
#include "ui/shell_dialogs/fake_select_file_dialog.h"

namespace screenshot {

namespace {

SkBitmap MakeSolidBitmap(int width, int height, SkColor color) {
  SkBitmap bm;
  bm.allocN32Pixels(width, height);
  SkCanvas canvas(bm);
  canvas.drawColor(color);
  return bm;
}

}  // namespace

using Error = ScreenshotController::Error;
using Result = base::expected<base::FilePath, Error>;

class ScreenshotControllerTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    // Install the fake select-file-dialog factory.  It intercepts every
    // SelectFileDialog::Create() call for the lifetime of this test.
    dialog_factory_ = ui::FakeSelectFileDialog::RegisterFactory();

    // Inject a simple download-dir getter so the controller never touches
    // DownloadPrefs (which requires a full download-service stack).
    const base::FilePath download_dir = temp_dir_.GetPath();
    controller_ = std::make_unique<ScreenshotController>(
        profile(), base::BindRepeating([]() { return gfx::NativeWindow(); }));
    controller_->set_download_dir_for_testing(download_dir);
  }

  void TearDown() override {
    controller_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  // Drives the pipeline from OnVisibleAreaCopied() onward, bypassing
  // CopyFromSurface (which returns kNotImplemented in the test environment).
  // Sets the controller into the same busy state that CaptureVisibleArea()
  // would set before dispatching CopyFromSurface.
  void InjectBitmap(SkBitmap bitmap, ScreenshotController::ResultCallback cb) {
    controller_->pending_callback_ = std::move(cb);
    controller_->OnVisibleAreaCopied(std::move(bitmap));
  }

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // Drives the pipeline from OnFullPageChunks() onward, bypassing
  // CaptureFullPage and the real PrintPreviewExtractor.
  void InjectChunks(
      base::expected<std::vector<std::vector<uint8_t>>, std::string> chunks,
      ScreenshotController::ResultCallback cb) {
    controller_->pending_callback_ = std::move(cb);
    controller_->OnFullPageChunks(std::move(chunks));
  }

  static std::vector<uint8_t> EncodeBitmapAsPng(const SkBitmap& bitmap) {
    auto encoded = gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false);
    CHECK(encoded);
    return *encoded;
  }
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

  void SetPendingCallback(ScreenshotController::ResultCallback cb) {
    controller_->pending_callback_ = std::move(cb);
  }

  base::ScopedTempDir temp_dir_;
  raw_ptr<ui::FakeSelectFileDialog::Factory> dialog_factory_ = nullptr;
  std::unique_ptr<ScreenshotController> controller_;
};

// ---------------------------------------------------------------------------
// CanCapture / early-reject tests (no bitmap injection needed)
// ---------------------------------------------------------------------------

TEST_F(ScreenshotControllerTest, CanCapture_ReturnsFalseForNullWebContents) {
  EXPECT_FALSE(controller_->CanCapture(nullptr).has_value());
}

TEST_F(ScreenshotControllerTest, CanCapture_ReturnsTrueWhenViewExists) {
  EXPECT_TRUE(controller_->CanCapture(web_contents()).has_value());
}

TEST_F(ScreenshotControllerTest, CanCapture_ReturnsBusyWhenPendingCallbackSet) {
  base::test::TestFuture<Result> pending_callback_future;
  SetPendingCallback(pending_callback_future.GetCallback());

  auto result = controller_->CanCapture(web_contents());
  EXPECT_EQ(result, base::unexpected(Error::kBusy));
}

TEST_F(ScreenshotControllerTest,
       CaptureVisibleArea_NullWebContents_ReturnsNoTab) {
  base::test::TestFuture<Result> future;
  controller_->CaptureVisibleArea(nullptr, future.GetCallback());
  EXPECT_EQ(future.Get(), base::unexpected(Error::kNoTab));
}

// Calling CaptureVisibleArea twice before the first completes: the second call
// should be rejected immediately with kBusy because the controller is busy.
TEST_F(ScreenshotControllerTest,
       CaptureVisibleArea_AlreadyBusy_SecondCallReturnsBusy) {
  base::test::TestFuture<Result> future1;
  base::test::TestFuture<Result> future2;

  // First call starts the async pipeline (CopyFromSurface is a no-op in tests;
  // its base-class implementation calls back with kNotImplemented immediately,
  // but the result is posted, not synchronous).
  controller_->CaptureVisibleArea(web_contents(), future1.GetCallback());

  // Second call sees busy_ == true and rejects immediately.
  controller_->CaptureVisibleArea(web_contents(), future2.GetCallback());
  EXPECT_EQ(future2.Get(), base::unexpected(Error::kBusy));

  // Let the first pipeline drain (kNotImplemented → empty bitmap →
  // kCaptureFailed).
  EXPECT_EQ(future1.Get(), base::unexpected(Error::kCaptureFailed));
}

// When CopyFromSurface fails (the test environment's base-class implementation
// returns kNotImplemented), the controller should report kCaptureFailed.
TEST_F(ScreenshotControllerTest,
       CaptureVisibleArea_CopyFromSurfaceFails_ReturnsCaptureFailed) {
  base::test::TestFuture<Result> future;
  controller_->CaptureVisibleArea(web_contents(), future.GetCallback());
  EXPECT_EQ(future.Get(), base::unexpected(Error::kCaptureFailed));
}

// ---------------------------------------------------------------------------
// Pipeline tests — inject a valid bitmap to reach the dialog / write stages
// ---------------------------------------------------------------------------

TEST_F(ScreenshotControllerTest, Pipeline_EmptyBitmap_ReturnsCaptureFailed) {
  base::test::TestFuture<Result> future;
  InjectBitmap(SkBitmap(), future.GetCallback());
  EXPECT_EQ(future.Get(), base::unexpected(Error::kCaptureFailed));
}

TEST_F(ScreenshotControllerTest,
       Pipeline_UserCancelsDialog_ReturnsUserCancelled) {
  // Signal when the fake dialog is opened so we can cancel it from outside
  // the callback (avoiding re-entrant dialog destruction).
  base::test::TestFuture<void> dialog_opened;
  dialog_factory_->SetOpenCallback(dialog_opened.GetRepeatingCallback());

  base::test::TestFuture<Result> future;
  InjectBitmap(MakeSolidBitmap(64, 64, SK_ColorBLUE), future.GetCallback());

  // Wait for the encode + path-build background tasks to complete and for
  // ShowSaveDialogWithPath to open the dialog.
  ASSERT_TRUE(dialog_opened.Wait());

  ui::FakeSelectFileDialog* dialog = dialog_factory_->GetLastDialog();
  ASSERT_TRUE(dialog);
  dialog->CallFileSelectionCanceled();

  EXPECT_EQ(future.Get(), base::unexpected(Error::kUserCancelled));
}

TEST_F(ScreenshotControllerTest,
       Pipeline_FileSelected_WritesToDiskAndReturnsPath) {
  base::test::TestFuture<void> dialog_opened;
  dialog_factory_->SetOpenCallback(dialog_opened.GetRepeatingCallback());

  base::test::TestFuture<Result> future;
  InjectBitmap(MakeSolidBitmap(64, 64, SK_ColorGREEN), future.GetCallback());

  ASSERT_TRUE(dialog_opened.Wait());

  ui::FakeSelectFileDialog* dialog = dialog_factory_->GetLastDialog();
  ASSERT_TRUE(dialog);

  base::FilePath save_path =
      temp_dir_.GetPath().AppendASCII("test_screenshot.png");
  ASSERT_TRUE(dialog->CallFileSelected(save_path, "png"));

  Result result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), save_path);
  EXPECT_TRUE(base::PathExists(save_path));
}

TEST_F(ScreenshotControllerTest,
       Pipeline_WriteToUnwritablePath_ReturnsWriteFailed) {
  base::test::TestFuture<void> dialog_opened;
  dialog_factory_->SetOpenCallback(dialog_opened.GetRepeatingCallback());

  base::test::TestFuture<Result> future;
  InjectBitmap(MakeSolidBitmap(64, 64, SK_ColorRED), future.GetCallback());

  ASSERT_TRUE(dialog_opened.Wait());

  ui::FakeSelectFileDialog* dialog = dialog_factory_->GetLastDialog();
  ASSERT_TRUE(dialog);

  // Select a path whose parent directory does not exist so WriteFile fails.
  base::FilePath bad_path = temp_dir_.GetPath()
                                .AppendASCII("nonexistent_subdir")
                                .AppendASCII("screenshot.png");
  ASSERT_TRUE(dialog->CallFileSelected(bad_path, "png"));

  EXPECT_EQ(future.Get(), base::unexpected(Error::kWriteFailed));
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)

// ---------------------------------------------------------------------------
// Full-page capture pipeline tests (OnFullPageChunks / StitchAndEncode)
// ---------------------------------------------------------------------------

TEST_F(ScreenshotControllerTest, FullPage_ExtractorError_ReturnsCaptureFailed) {
  base::test::TestFuture<Result> future;
  InjectChunks(base::unexpected(std::string("extractor failed")),
               future.GetCallback());
  EXPECT_EQ(future.Get(), base::unexpected(Error::kCaptureFailed));
}

// An empty chunks vector causes StitchAndEncode to return nullopt, which the
// controller surfaces as kEncodeFailed.
TEST_F(ScreenshotControllerTest, FullPage_EmptyChunks_ReturnsEncodeFailed) {
  base::test::TestFuture<Result> future;
  InjectChunks(std::vector<std::vector<uint8_t>>{}, future.GetCallback());
  EXPECT_EQ(future.Get(), base::unexpected(Error::kEncodeFailed));
}

TEST_F(ScreenshotControllerTest, FullPage_SingleChunk_SavesToDisk) {
  base::test::TestFuture<void> dialog_opened;
  dialog_factory_->SetOpenCallback(dialog_opened.GetRepeatingCallback());

  std::vector<std::vector<uint8_t>> chunks = {
      EncodeBitmapAsPng(MakeSolidBitmap(64, 48, SK_ColorBLUE))};

  base::test::TestFuture<Result> future;
  InjectChunks(std::move(chunks), future.GetCallback());

  ASSERT_TRUE(dialog_opened.Wait());
  ui::FakeSelectFileDialog* dialog = dialog_factory_->GetLastDialog();
  ASSERT_TRUE(dialog);

  base::FilePath save_path =
      temp_dir_.GetPath().AppendASCII("fullpage_single.png");
  ASSERT_TRUE(dialog->CallFileSelected(save_path, "png"));

  Result result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), save_path);
  EXPECT_TRUE(base::PathExists(save_path));
}

// Two chunks of different heights should be stitched into an image whose width
// is the maximum chunk width and whose height is the sum of the chunk heights.
TEST_F(ScreenshotControllerTest,
       FullPage_MultiChunk_StitchesToCorrectDimensions) {
  base::test::TestFuture<void> dialog_opened;
  dialog_factory_->SetOpenCallback(dialog_opened.GetRepeatingCallback());

  // Chunk 0: 64×48, chunk 1: 64×32 → stitched: 64×80.
  std::vector<std::vector<uint8_t>> chunks = {
      EncodeBitmapAsPng(MakeSolidBitmap(64, 48, SK_ColorRED)),
      EncodeBitmapAsPng(MakeSolidBitmap(64, 32, SK_ColorGREEN))};

  base::test::TestFuture<Result> future;
  InjectChunks(std::move(chunks), future.GetCallback());

  ASSERT_TRUE(dialog_opened.Wait());
  ui::FakeSelectFileDialog* dialog = dialog_factory_->GetLastDialog();
  ASSERT_TRUE(dialog);

  base::FilePath save_path =
      temp_dir_.GetPath().AppendASCII("fullpage_multi.png");
  ASSERT_TRUE(dialog->CallFileSelected(save_path, "png"));

  Result result = future.Get();
  ASSERT_TRUE(result.has_value());

  std::optional<std::vector<uint8_t>> png_bytes =
      base::ReadFileToBytes(save_path);
  ASSERT_TRUE(png_bytes);
  SkBitmap stitched = gfx::PNGCodec::Decode(*png_bytes);
  EXPECT_EQ(stitched.width(), 64);
  EXPECT_EQ(stitched.height(), 80);  // 48 + 32
  // Verify vertical ordering: chunk 0 (red) is drawn above chunk 1 (green).
  EXPECT_EQ(stitched.getColor(0, 0), SK_ColorRED);
  EXPECT_EQ(stitched.getColor(0, 48), SK_ColorGREEN);
}

#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

}  // namespace screenshot

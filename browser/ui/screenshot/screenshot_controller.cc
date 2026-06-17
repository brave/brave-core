// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshot/screenshot_controller.h"

#include <algorithm>
#include <utility>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/download/download_prefs.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "brave/browser/screenshot/print_preview_extractor.h"
#endif

namespace screenshot {

namespace {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
// Cap to avoid OOM on extremely tall stitched pages.
constexpr int64_t kMaxStitchedBytes = 512LL * 1024 * 1024;

std::optional<std::vector<uint8_t>> StitchAndEncode(
    std::vector<std::vector<uint8_t>> chunks) {
  if (chunks.empty()) {
    return std::nullopt;
  }

  std::vector<SkBitmap> opaque_chunks;
  opaque_chunks.reserve(chunks.size());
  for (const auto& png : chunks) {
    SkBitmap bmp = gfx::PNGCodec::Decode(png);
    if (bmp.drawsNothing()) {
      return std::nullopt;
    }
    SkIRect bounds = bmp.bounds();
    if (bounds.isEmpty()) {
      continue;
    }
    opaque_chunks.push_back(std::move(bmp));
  }
  if (opaque_chunks.empty()) {
    return std::nullopt;
  }

  int width = 0;
  int total_height = 0;
  for (const auto& bitmap : opaque_chunks) {
    width = std::max(width, bitmap.bounds().width());
    total_height += bitmap.bounds().height();
  }
  if (width <= 0 || total_height <= 0) {
    return std::nullopt;
  }
  if (static_cast<int64_t>(width) * total_height * 4 > kMaxStitchedBytes) {
    return std::nullopt;
  }

  SkBitmap out;
  if (!out.tryAllocN32Pixels(width, total_height)) {
    return std::nullopt;
  }
  SkCanvas canvas(out);
  canvas.clear(SK_ColorWHITE);
  int y_offset = 0;
  for (const auto& bitmap : opaque_chunks) {
    // Draw only the opaque region of the chunk at (0, y_offset).
    SkRect src = SkRect::Make(bitmap.bounds());
    SkRect dst = SkRect::MakeXYWH(0, y_offset, bitmap.bounds().width(),
                                  bitmap.bounds().height());
    canvas.drawImageRect(bitmap.asImage(), src, dst, SkSamplingOptions(),
                         /*paint=*/nullptr,
                         SkCanvas::kStrict_SrcRectConstraint);
    y_offset += bitmap.bounds().height();
  }

  return gfx::PNGCodec::EncodeBGRASkBitmap(out, /*discard_transparency=*/true);
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

std::optional<std::vector<uint8_t>> EncodeBitmap(const SkBitmap& bitmap) {
  if (bitmap.drawsNothing()) {
    return std::nullopt;
  }
  return gfx::PNGCodec::EncodeBGRASkBitmap(bitmap,
                                           /*discard_transparency=*/true);
}

bool WritePngFile(const base::FilePath& path, base::span<const uint8_t> bytes) {
  return base::WriteFile(path, bytes);
}

base::FilePath BuildDefaultPath(const base::FilePath& download_dir) {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);
  std::string name = absl::StrFormat(
      "screenshot-%04d-%02d-%02d-%02d%02d%02d.png", now.year, now.month,
      now.day_of_month, now.hour, now.minute, now.second);
  return base::GetUniquePath(download_dir.AppendASCII(name));
}

}  // namespace

ScreenshotController::ScreenshotController(
    content::BrowserContext* profile,
    NativeWindowGetter parent_window_getter)
    : parent_window_getter_(std::move(parent_window_getter)),
      profile_(profile) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
ScreenshotController::ScreenshotController(
    content::BrowserContext* profile,
    NativeWindowGetter parent_window_getter,
    std::unique_ptr<screenshot::PrintPreviewExtractor> print_preview_extractor)
    : parent_window_getter_(std::move(parent_window_getter)),
      print_preview_extractor_(std::move(print_preview_extractor)),
      profile_(profile) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  CHECK(print_preview_extractor_);
}
#endif

ScreenshotController::~ScreenshotController() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (select_dialog_) {
    select_dialog_->ListenerDestroyed();
  }
}

bool ScreenshotController::CanCapture(
    content::WebContents* web_contents) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (busy_ || !web_contents) {
    return false;
  }
  return web_contents->GetRenderWidgetHostView() != nullptr;
}

void ScreenshotController::CaptureVisibleArea(
    content::WebContents* web_contents,
    ResultCallback done) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!CanCapture(web_contents)) {
    std::move(done).Run(base::unexpected(Error::kNoTab));
    return;
  }
  auto* view = web_contents->GetRenderWidgetHostView();
  CHECK(view);

  busy_ = true;
  pending_callback_ = std::move(done);

  // CopyFromSurface's callback isn't guaranteed to run on the calling sequence,
  // so post back to the UI thread before touching member state.
  auto on_copied = base::BindPostTaskToCurrentDefault(base::BindOnce(
      [](base::WeakPtr<ScreenshotController> self,
         const content::CopyFromSurfaceResult& result) {
        if (!self) {
          return;
        }
        SkBitmap bitmap;
        if (result.has_value()) {
          bitmap = result.value().bitmap;
        }
        self->OnVisibleAreaCopied(std::move(bitmap));
      },
      weak_factory_.GetWeakPtr()));

  view->CopyFromSurface(gfx::Rect(), gfx::Size(), base::TimeDelta(),
                        std::move(on_copied));
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void ScreenshotController::CaptureFullPage(content::WebContents* web_contents,
                                           ResultCallback done) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!CanCapture(web_contents)) {
    std::move(done).Run(base::unexpected(Error::kNoTab));
    return;
  }

  busy_ = true;
  pending_callback_ = std::move(done);

  auto on_chunks = base::BindOnce(&ScreenshotController::OnFullPageChunks,
                                  weak_factory_.GetWeakPtr());
  print_preview_extractor_->CaptureImages(web_contents, std::move(on_chunks));
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

void ScreenshotController::OnVisibleAreaCopied(SkBitmap bitmap) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (bitmap.drawsNothing()) {
    FinishWithError(Error::kCaptureFailed);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&EncodeBitmap, std::move(bitmap)),
      base::BindOnce(&ScreenshotController::OnEncoded,
                     weak_factory_.GetWeakPtr()));
}

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
void ScreenshotController::OnFullPageChunks(ChunkResult result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!result.has_value()) {
    DVLOG(1) << "Full page capture failed: " << result.error();
    FinishWithError(Error::kCaptureFailed);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&StitchAndEncode, std::move(result.value())),
      base::BindOnce(&ScreenshotController::OnEncoded,
                     weak_factory_.GetWeakPtr()));
}
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

void ScreenshotController::OnEncoded(std::optional<std::vector<uint8_t>> png) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!png) {
    FinishWithError(Error::kEncodeFailed);
    return;
  }
  ShowSaveDialog(std::move(*png));
}

void ScreenshotController::ShowSaveDialog(std::vector<uint8_t> png) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  pending_png_ = std::move(png);
  base::FilePath download_dir =
      download_dir_for_testing_.value_or(base::FilePath());
  if (download_dir.empty()) {
    download_dir = DownloadPrefs::FromBrowserContext(profile_)->DownloadPath();
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&BuildDefaultPath, download_dir),
      base::BindOnce(&ScreenshotController::ShowSaveDialogWithPath,
                     weak_factory_.GetWeakPtr()));
}

void ScreenshotController::ShowSaveDialogWithPath(base::FilePath default_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (default_path.empty()) {
    FinishWithError(Error::kWriteFailed);
    return;
  }

  if (select_dialog_) {
    select_dialog_->ListenerDestroyed();
  }
  select_dialog_ = ui::SelectFileDialog::Create(this, nullptr);

  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.extensions.push_back({FILE_PATH_LITERAL("png")});

  select_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_SAVEAS_FILE,
      l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_BUBBLE_TITLE),
      default_path, &file_types, /*file_type_index=*/1,
      FILE_PATH_LITERAL("png"), parent_window_getter_.Run(),
      /*params=*/nullptr);
}

void ScreenshotController::FileSelected(const ui::SelectedFileInfo& file,
                                        int /*index*/) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::FilePath path = file.file_path;
  // Move bytes out of member state before posting so a re-entrant call after
  // Reset() doesn't see them.
  std::vector<uint8_t> bytes = std::move(pending_png_);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&WritePngFile, path, std::move(bytes)),
      base::BindOnce(&ScreenshotController::OnFileWritten,
                     weak_factory_.GetWeakPtr(), path));
}

void ScreenshotController::OnFileWritten(base::FilePath path, bool ok) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ResultCallback cb = std::move(pending_callback_);
  Reset();
  if (!cb) {
    return;
  }
  if (ok) {
    std::move(cb).Run(path);
  } else {
    std::move(cb).Run(base::unexpected(Error::kWriteFailed));
  }
}

void ScreenshotController::FileSelectionCanceled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  FinishWithError(Error::kUserCancelled);
}

void ScreenshotController::FinishWithError(Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ResultCallback cb = std::move(pending_callback_);
  Reset();
  if (cb) {
    std::move(cb).Run(base::unexpected(error));
  }
}

void ScreenshotController::Reset() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  busy_ = false;
  pending_png_.clear();
  if (select_dialog_) {
    select_dialog_->ListenerDestroyed();
    select_dialog_.reset();
  }
}

}  // namespace screenshot

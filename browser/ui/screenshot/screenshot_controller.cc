// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshot/screenshot_controller.h"

#include <algorithm>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/content/browser/full_screenshotter.h"
#include "brave/components/ai_chat/content/browser/pdf_utils.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
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
#include "brave/browser/print_preview_extraction/print_preview_extractor.h"
#include "brave/browser/print_preview_extraction/print_preview_extractor_factory.h"
#endif

namespace brave {

namespace {

// Cap to avoid OOM on extremely tall stitched pages.
constexpr int64_t kMaxStitchedBytes = 512LL * 1024 * 1024;

std::optional<std::vector<uint8_t>> EncodeBitmap(const SkBitmap& bitmap) {
  if (bitmap.drawsNothing()) {
    return std::nullopt;
  }
  return gfx::PNGCodec::EncodeBGRASkBitmap(bitmap,
                                           /*discard_transparency=*/true);
}

// Returns the bounding box of pixels with non-zero alpha. Both
// `ai_chat::FullScreenshotter` and `ai_chat::PrintPreviewExtractor` pipe
// captures through `ai_chat::ScaleDownBitmap`, which centers the scaled
// content inside a fixed 1024x768 canvas and fills the remainder with
// SkColorTRANSPARENT. We crop that padding off before stitching, otherwise
// every chunk contributes empty vertical (or horizontal) bands.
SkIRect FindOpaqueBounds(const SkBitmap& bm) {
  if (bm.empty() || bm.colorType() != kN32_SkColorType) {
    return SkIRect::MakeWH(bm.width(), bm.height());
  }
  int top = -1;
  int bottom = -1;
  int left = bm.width();
  int right = -1;
  for (int y = 0; y < bm.height(); ++y) {
    int row_left = -1;
    int row_right = -1;
    for (int x = 0; x < bm.width(); ++x) {
      if (SkColorGetA(*bm.getAddr32(x, y)) > 0) {
        if (row_left < 0) {
          row_left = x;
        }
        row_right = x;
      }
    }
    if (row_left >= 0) {
      if (top < 0) {
        top = y;
      }
      bottom = y;
      left = std::min(left, row_left);
      right = std::max(right, row_right);
    }
  }
  if (top < 0) {
    return SkIRect::MakeEmpty();
  }
  return SkIRect::MakeLTRB(left, top, right + 1, bottom + 1);
}

std::optional<std::vector<uint8_t>> StitchAndEncode(
    std::vector<std::vector<uint8_t>> chunks) {
  if (chunks.empty()) {
    return std::nullopt;
  }

  struct Cropped {
    SkBitmap bitmap;
    SkIRect bounds;  // region within `bitmap` that holds opaque content
  };

  std::vector<Cropped> cropped;
  cropped.reserve(chunks.size());
  for (const auto& png : chunks) {
    SkBitmap bmp = gfx::PNGCodec::Decode(png);
    if (bmp.drawsNothing()) {
      return std::nullopt;
    }
    SkIRect bounds = FindOpaqueBounds(bmp);
    if (bounds.isEmpty()) {
      continue;  // entirely transparent chunk — skip
    }
    cropped.push_back({std::move(bmp), bounds});
  }
  if (cropped.empty()) {
    return std::nullopt;
  }

  int width = 0;
  int total_height = 0;
  for (const auto& c : cropped) {
    width = std::max(width, c.bounds.width());
    total_height += c.bounds.height();
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
  for (const auto& c : cropped) {
    // Draw only the opaque region of the chunk at (0, y_offset).
    SkRect src = SkRect::Make(c.bounds);
    SkRect dst =
        SkRect::MakeXYWH(0, y_offset, c.bounds.width(), c.bounds.height());
    canvas.drawImageRect(c.bitmap.asImage(), src, dst, SkSamplingOptions(),
                         /*paint=*/nullptr,
                         SkCanvas::kStrict_SrcRectConstraint);
    y_offset += c.bounds.height();
  }

  return gfx::PNGCodec::EncodeBGRASkBitmap(out, /*discard_transparency=*/true);
}

bool WritePngFile(const base::FilePath& path,
                  const std::vector<uint8_t>& bytes) {
  return base::WriteFile(path, bytes);
}

base::FilePath BuildDefaultPath(const base::FilePath& download_dir) {
  base::Time::Exploded now;
  base::Time::Now().LocalExplode(&now);
  std::string name = base::StringPrintf(
      "screenshot-%04d-%02d-%02d-%02d%02d%02d.png", now.year, now.month,
      now.day_of_month, now.hour, now.minute, now.second);
  return base::GetUniquePath(download_dir.AppendASCII(name));
}

}  // namespace

ScreenshotController::ScreenshotController(
    Profile* profile,
    NativeWindowGetter parent_window_getter)
    : profile_(profile),
      parent_window_getter_(std::move(parent_window_getter)) {}

ScreenshotController::~ScreenshotController() {
  if (select_dialog_) {
    select_dialog_->ListenerDestroyed();
  }
}

bool ScreenshotController::CanCapture(content::WebContents* wc) const {
  if (busy_ || !wc) {
    return false;
  }
  return wc->GetRenderWidgetHostView() != nullptr;
}

void ScreenshotController::CaptureVisibleArea(content::WebContents* wc,
                                              ResultCallback done) {
  if (!CanCapture(wc)) {
    std::move(done).Run(base::unexpected(Error::kNoTab));
    return;
  }
  auto* view = wc->GetRenderWidgetHostView();
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

void ScreenshotController::CaptureFullPage(content::WebContents* wc,
                                           ResultCallback done) {
  if (!CanCapture(wc)) {
    std::move(done).Run(base::unexpected(Error::kNoTab));
    return;
  }

  busy_ = true;
  pending_callback_ = std::move(done);

  auto on_chunks = base::BindOnce(&ScreenshotController::OnFullPageChunks,
                                  weak_factory_.GetWeakPtr());

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  const bool use_print_preview =
      ai_chat::IsPdf(wc) || ai_chat::kPrintPreviewRetrievalHosts.contains(
                                wc->GetLastCommittedURL().host());
  if (use_print_preview) {
    print_preview_extractor_ = ai_chat::CreateDefaultPrintPreviewExtractor(wc);
    print_preview_extractor_->CaptureImages(std::move(on_chunks));
    return;
  }
#endif

  full_screenshotter_ = std::make_unique<ai_chat::FullScreenshotter>();
  full_screenshotter_->CaptureScreenshots(wc, std::move(on_chunks));
}

void ScreenshotController::OnVisibleAreaCopied(SkBitmap bitmap) {
  if (bitmap.drawsNothing()) {
    FinishWithError(Error::kCaptureFailed);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&EncodeBitmap, std::move(bitmap)),
      base::BindOnce(&ScreenshotController::OnEncoded,
                     weak_factory_.GetWeakPtr()));
}

void ScreenshotController::OnFullPageChunks(ChunkResult result) {
  // Note: the extractor that just produced `result` is still on the call stack
  // (this callback is invoked from inside its OnComplete). Don't destroy it
  // here — Reset() will clean it up after the full flow finishes.
  if (!result.has_value()) {
    VLOG(1) << "Full page capture failed: " << result.error();
    FinishWithError(Error::kCaptureFailed);
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&StitchAndEncode, std::move(result.value())),
      base::BindOnce(&ScreenshotController::OnEncoded,
                     weak_factory_.GetWeakPtr()));
}

void ScreenshotController::OnEncoded(std::optional<std::vector<uint8_t>> png) {
  if (!png) {
    FinishWithError(Error::kEncodeFailed);
    return;
  }
  ShowSaveDialog(std::move(*png));
}

void ScreenshotController::ShowSaveDialog(std::vector<uint8_t> png) {
  pending_png_ = std::move(png);
  base::FilePath download_dir =
      DownloadPrefs::FromBrowserContext(profile_)->DownloadPath();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&BuildDefaultPath, download_dir),
      base::BindOnce(&ScreenshotController::ShowSaveDialogWithPath,
                     weak_factory_.GetWeakPtr()));
}

void ScreenshotController::ShowSaveDialogWithPath(base::FilePath default_path) {
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
  base::FilePath path = file.file_path;
  // Move bytes out of member state before posting so a re-entrant call after
  // Reset() doesn't see them.
  std::vector<uint8_t> bytes = std::move(pending_png_);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&WritePngFile, path, std::move(bytes)),
      base::BindOnce(
          [](base::WeakPtr<ScreenshotController> self, base::FilePath path,
             bool ok) {
            if (!self) {
              return;
            }
            ResultCallback cb = std::move(self->pending_callback_);
            self->Reset();
            if (!cb) {
              return;
            }
            if (ok) {
              std::move(cb).Run(path);
            } else {
              std::move(cb).Run(base::unexpected(Error::kWriteFailed));
            }
          },
          weak_factory_.GetWeakPtr(), path));
}

void ScreenshotController::FileSelectionCanceled() {
  FinishWithError(Error::kUserCancelled);
}

void ScreenshotController::FinishWithError(Error error) {
  ResultCallback cb = std::move(pending_callback_);
  Reset();
  if (cb) {
    std::move(cb).Run(base::unexpected(error));
  }
}

void ScreenshotController::Reset() {
  busy_ = false;
  pending_png_.clear();
  if (select_dialog_) {
    select_dialog_->ListenerDestroyed();
    select_dialog_.reset();
  }
  // DeleteSoon — the extractor that invoked our callback may still be on the
  // call stack when Reset() runs.
  if (full_screenshotter_) {
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(full_screenshotter_));
  }
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  if (print_preview_extractor_) {
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(print_preview_extractor_));
  }
#endif
}

}  // namespace brave

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/full_screenshotter.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "components/paint_preview/browser/compositor_utils.h"
#include "components/paint_preview/browser/paint_preview_base_service.h"
#include "components/paint_preview/common/recording_map.h"
#include "content/public/browser/render_widget_host_view.h"
#include "mojo/public/cpp/base/proto_wrapper.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"

namespace ai_chat {

namespace {

int g_debug_file_sequencer = 0;

void WriteBitmapToPng(const SkBitmap& bitmap) {
  base::FilePath temp_dir = base::PathService::CheckedGet(base::DIR_TEMP);
  const auto debug_screenshot_dir =
      temp_dir.AppendASCII("brave_debug_screenshots");
  if (!g_debug_file_sequencer) {
    base::DeletePathRecursively(debug_screenshot_dir);
    LOG(ERROR) << "debug_screenshot_dir: " << debug_screenshot_dir.value();
    base::CreateDirectory(debug_screenshot_dir);
  }
  std::string screenshot_filename =
      base::StringPrintf("fullscreenshot_%i.png", g_debug_file_sequencer++);
  std::string screenshot_filepath =
      debug_screenshot_dir.AppendASCII(screenshot_filename).MaybeAsASCII();
  SkFILEWStream out_file(screenshot_filepath.c_str());
  if (!out_file.isValid()) {
    VLOG(2) << "Unable to create: " << screenshot_filepath;
    return;
  }

  bool success =
      SkPngEncoder::Encode(&out_file, bitmap.pixmap(), /*options=*/{});
  if (success) {
    VLOG(2) << "Wrote debug file: " << screenshot_filepath;
  } else {
    VLOG(2) << "Failed to write debug file: " << screenshot_filepath;
  }
}

}  // namespace

FullScreenshotter::FullScreenshotter()
    : paint_preview::PaintPreviewBaseService(
          /*file_mixin=*/nullptr,  // in-memory captures
          /*policy=*/nullptr,      // all content is deemed amenable
          /*is_off_the_record=*/true),
      paint_preview_compositor_service_(nullptr,
                                        base::OnTaskRunnerDeleter(nullptr)),
      paint_preview_compositor_client_(nullptr,
                                       base::OnTaskRunnerDeleter(nullptr)) {
  paint_preview_compositor_service_ = paint_preview::StartCompositorService(
      base::BindOnce(&FullScreenshotter::OnCompositorServiceDisconnected,
                     weak_ptr_factory_.GetWeakPtr()));
  CHECK(paint_preview_compositor_service_);
}

FullScreenshotter::~FullScreenshotter() = default;

FullScreenshotter::PendingScreenshots::PendingScreenshots() = default;
FullScreenshotter::PendingScreenshots::~PendingScreenshots() = default;

void FullScreenshotter::CaptureScreenshot(
    const raw_ptr<content::WebContents> web_contents,
    CaptureScreenshotCallback callback) {
  current_web_contents_ = web_contents;
  if (!web_contents) {
    std::move(callback).Run(
        base::unexpected("The given web contents no longer valid"));
    return;
  }

  // Start capturing via Paint Preview.
  CaptureParams capture_params;
  capture_params.web_contents = web_contents;
  capture_params.persistence =
      paint_preview::RecordingPersistence::kMemoryBuffer;
  CapturePaintPreview(
      capture_params,
      base::BindOnce(&FullScreenshotter::OnScreenshotCaptured,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FullScreenshotter::OnScreenshotCaptured(
    CaptureScreenshotCallback callback,
    paint_preview::PaintPreviewBaseService::CaptureStatus status,
    std::unique_ptr<paint_preview::CaptureResult> result) {
  if (status != PaintPreviewBaseService::CaptureStatus::kOk ||
      !result->capture_success) {
    std::move(callback).Run(base::unexpected(
        base::StringPrintf("Failed to capture a screenshot (CaptureStatus=%d)",
                           static_cast<int>(status))));
    return;
  }

  g_debug_file_sequencer = 0;
  if (!paint_preview_compositor_client_) {
    paint_preview_compositor_client_ =
        paint_preview_compositor_service_->CreateCompositor(
            base::BindOnce(&FullScreenshotter::SendCompositeRequest,
                           weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                           PrepareCompositeRequest(std::move(result))));
  } else {
    SendCompositeRequest(std::move(callback),
                         PrepareCompositeRequest(std::move(result)));
  }
}

paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr
FullScreenshotter::PrepareCompositeRequest(
    std::unique_ptr<paint_preview::CaptureResult> capture_result) {
  paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr
      begin_composite_request =
          paint_preview::mojom::PaintPreviewBeginCompositeRequest::New();
  std::pair<paint_preview::RecordingMap, paint_preview::PaintPreviewProto>
      map_and_proto = paint_preview::RecordingMapFromCaptureResult(
          std::move(*capture_result));
  begin_composite_request->recording_map = std::move(map_and_proto.first);
  if (begin_composite_request->recording_map.empty()) {
    VLOG(2) << "Captured an empty screenshot";
    return nullptr;
  }
  begin_composite_request->preview =
      mojo_base::ProtoWrapper(std::move(map_and_proto.second));
  return begin_composite_request;
}

void FullScreenshotter::SendCompositeRequest(
    CaptureScreenshotCallback callback,
    paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr
        begin_composite_request) {
  if (!begin_composite_request) {
    std::move(callback).Run(
        base::unexpected("Invalid begin_composite_request"));
    return;
  }

  CHECK(paint_preview_compositor_client_);
  paint_preview_compositor_client_->BeginMainFrameComposite(
      std::move(begin_composite_request),
      base::BindOnce(&FullScreenshotter::OnCompositeFinished,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FullScreenshotter::OnCompositorServiceDisconnected() {
  VLOG(2) << "Compositor service is disconnected";
  paint_preview_compositor_client_.reset();
  paint_preview_compositor_service_.reset();
}

void FullScreenshotter::OnCompositeFinished(
    CaptureScreenshotCallback callback,
    paint_preview::mojom::PaintPreviewCompositor::BeginCompositeStatus status,
    paint_preview::mojom::PaintPreviewBeginCompositeResponsePtr response) {
  if (status != paint_preview::mojom::PaintPreviewCompositor::
                    BeginCompositeStatus::kSuccess) {
    std::move(callback).Run(base::unexpected("Failed to begin composite"));
    return;
  }

  auto* view = current_web_contents_->GetRenderWidgetHostView();
  if (!view) {
    std::move(callback).Run(
        base::unexpected("No render widget host view available"));
    return;
  }

  const auto& frame_data = response->frames[response->root_frame_guid];
  const auto& content_size = frame_data->scroll_extents;
  const auto viewport_bounds = view->GetVisibleViewportSize();

  auto pending = std::make_unique<PendingScreenshots>();

  // Calculate number of full viewport screenshots needed
  int total_height = content_size.height();
  int viewport_height = viewport_bounds.height();
  int num_screenshots = (total_height + viewport_height - 1) /
                        viewport_height;  // Ceiling division

  pending->completed_images.resize(num_screenshots);

  // Queue up screenshot rectangles
  for (int i = 0; i < num_screenshots; ++i) {
    int y = i * viewport_height;
    int height = std::min(viewport_height, total_height - y);
    pending->remaining_rects.emplace(0, y, content_size.width(), height);
  }

  pending->callback = std::move(callback);
  CaptureNextScreenshot(std::move(pending));
}

void FullScreenshotter::CaptureNextScreenshot(
    std::unique_ptr<PendingScreenshots> pending) {
  if (pending->remaining_rects.empty()) {
    // All screenshots captured, return results
    std::move(pending->callback)
        .Run(base::ok(std::move(pending->completed_images)));
    return;
  }

  // Take the next rect to capture
  gfx::Rect capture_rect = pending->remaining_rects.front();
  pending->remaining_rects.pop();

  LOG(ERROR) << capture_rect.ToString();
  paint_preview_compositor_client_->BitmapForMainFrame(
      capture_rect, 1,
      base::BindOnce(&FullScreenshotter::OnBitmapReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(pending),
                     pending->completed_images.size() -
                         pending->remaining_rects.size() - 1));
}

void FullScreenshotter::OnBitmapReceived(
    std::unique_ptr<PendingScreenshots> pending,
    size_t index,
    paint_preview::mojom::PaintPreviewCompositor::BitmapStatus status,
    const SkBitmap& bitmap) {
  if (status != paint_preview::mojom::PaintPreviewCompositor::BitmapStatus::
                    kSuccess ||
      bitmap.empty()) {
    std::move(pending->callback)
        .Run(base::unexpected(
            base::StringPrintf("Failed to get bitmap (BitmapStatus=%d)",
                               static_cast<int>(status))));
    return;
  }

  constexpr int kTargetWidth = 1024;
  constexpr int kTargetHeight = 768;

  SkBitmap scaled_bitmap;
  scaled_bitmap.allocN32Pixels(kTargetWidth, kTargetHeight);

  SkCanvas canvas(scaled_bitmap);
  canvas.clear(SK_ColorTRANSPARENT);

  // Use high-quality scaling options
  SkSamplingOptions sampling_options(SkFilterMode::kLinear,
                                     SkMipmapMode::kLinear);

  // Maintain aspect ratio while fitting within target dimensions
  float src_aspect = static_cast<float>(bitmap.width()) / bitmap.height();
  float dst_aspect = static_cast<float>(kTargetWidth) / kTargetHeight;

  SkRect dst_rect;
  if (src_aspect > dst_aspect) {
    // Source is wider - fit to width
    float scaled_height = kTargetWidth / src_aspect;
    float y_offset = (kTargetHeight - scaled_height) / 2;
    dst_rect = SkRect::MakeXYWH(0, y_offset, kTargetWidth, scaled_height);
  } else {
    // Source is taller - fit to height
    float scaled_width = kTargetHeight * src_aspect;
    float x_offset = (kTargetWidth - scaled_width) / 2;
    dst_rect = SkRect::MakeXYWH(x_offset, 0, scaled_width, kTargetHeight);
  }

  // Draw scaled bitmap with high-quality sampling
  canvas.drawImageRect(bitmap.asImage(), dst_rect, sampling_options);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&FullScreenshotter::EncodeBitmap, scaled_bitmap),
      base::BindOnce(&FullScreenshotter::OnBitmapEncoded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(pending),
                     index));
}

// static
base::expected<std::string, std::string> FullScreenshotter::EncodeBitmap(
    const SkBitmap& bitmap) {
  WriteBitmapToPng(bitmap);
  auto encoded_result = gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false);
  if (!encoded_result) {
    return base::unexpected("Failed to encode the bitmap");
  }
  
  std::vector<unsigned char> data = std::move(encoded_result.value());
  return base::ok(base::Base64Encode(data));
}

void FullScreenshotter::OnBitmapEncoded(
    std::unique_ptr<PendingScreenshots> pending,
    size_t index,
    base::expected<std::string, std::string> result) {
  if (!result.has_value()) {
    std::move(pending->callback).Run(base::unexpected(result.error()));
    return;
  }
  pending->completed_images[index] = std::move(*result);
  CaptureNextScreenshot(std::move(pending));
}

}  // namespace ai_chat

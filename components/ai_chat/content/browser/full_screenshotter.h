/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_FULL_SCREENSHOTTER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_FULL_SCREENSHOTTER_H_

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "components/paint_preview/browser/paint_preview_base_service.h"
#include "components/paint_preview/public/paint_preview_compositor_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"

namespace ai_chat {

class FullScreenshotter : public paint_preview::PaintPreviewBaseService {
 public:
  FullScreenshotter();
  FullScreenshotter(const FullScreenshotter&) = delete;
  FullScreenshotter& operator=(const FullScreenshotter&) = delete;
  ~FullScreenshotter() override;

  // <base64 encoded images, error>
  using CaptureScreenshotCallback = base::OnceCallback<void(
      base::expected<std::vector<std::string>, std::string>)>;
  void CaptureScreenshot(const raw_ptr<content::WebContents> web_contents,
                         CaptureScreenshotCallback callback);

 private:
  void OnScreenshotCaptured(
      CaptureScreenshotCallback callback,
      paint_preview::PaintPreviewBaseService::CaptureStatus status,
      std::unique_ptr<paint_preview::CaptureResult> result);
  void OnCompositorServiceDisconnected();
  paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr
  PrepareCompositeRequest(
      std::unique_ptr<paint_preview::CaptureResult> capture_result);
  void SendCompositeRequest(
      CaptureScreenshotCallback callback,
      paint_preview::mojom::PaintPreviewBeginCompositeRequestPtr
          begin_composite_request);
  void RequestBitmapForMainFrame(CaptureScreenshotCallback callback);
  void OnCompositeFinished(
      CaptureScreenshotCallback callback,
      paint_preview::mojom::PaintPreviewCompositor::BeginCompositeStatus status,
      paint_preview::mojom::PaintPreviewBeginCompositeResponsePtr response);

  struct PendingScreenshots {
    PendingScreenshots();
    ~PendingScreenshots();
    std::queue<gfx::Rect> remaining_rects;
    std::vector<std::string> completed_images;
    CaptureScreenshotCallback callback;
  };

  static base::expected<std::string, std::string> EncodeBitmap(
      const SkBitmap& bitmap);
  void OnBitmapReceived(
      std::unique_ptr<PendingScreenshots> pending,
      size_t index,
      paint_preview::mojom::PaintPreviewCompositor::BitmapStatus status,
      const SkBitmap& bitmap);
  void OnBitmapEncoded(std::unique_ptr<PendingScreenshots> pending,
                       size_t index,
                       base::expected<std::string, std::string> result);

  void CaptureNextScreenshot(std::unique_ptr<PendingScreenshots> pending);

  std::unique_ptr<paint_preview::PaintPreviewCompositorService,
                  base::OnTaskRunnerDeleter>
      paint_preview_compositor_service_;
  std::unique_ptr<paint_preview::PaintPreviewCompositorClient,
                  base::OnTaskRunnerDeleter>
      paint_preview_compositor_client_;

  raw_ptr<content::WebContents> current_web_contents_;

  base::WeakPtrFactory<FullScreenshotter> weak_ptr_factory_{this};
};

}  // namespace ai_chat
#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_FULL_SCREENSHOTTER_H_

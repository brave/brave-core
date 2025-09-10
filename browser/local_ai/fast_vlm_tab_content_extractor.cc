// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/fast_vlm_tab_content_extractor.h"

#include "base/logging.h"
#include "brave/browser/local_ai/fast_vlm_service.h"
#include "brave/browser/local_ai/fast_vlm_service_factory.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace local_ai {

FastVLMTabContentExtractor::FastVLMTabContentExtractor() = default;
FastVLMTabContentExtractor::~FastVLMTabContentExtractor() = default;

// static
void FastVLMTabContentExtractor::ExtractVisualContent(
    content::WebContents* web_contents,
    int tab_index,
    ExtractContentCallback callback) {
  if (!web_contents) {
    LOG(ERROR) << "WebContents is null for tab " << tab_index;
    std::move(callback).Run(tab_index, "");
    return;
  }

  LOG(INFO) << "Extracting visual content for tab " << tab_index
            << " URL: " << web_contents->GetVisibleURL().spec();

  // Get the render widget host view to capture the surface
  content::RenderWidgetHostView* view = web_contents->GetRenderWidgetHostView();
  if (!view) {
    LOG(ERROR) << "RenderWidgetHostView is null for tab " << tab_index;
    std::move(callback).Run(tab_index, "");
    return;
  }

  // Use CopyFromSurface to capture the rendered viewport
  view->CopyFromSurface(
      gfx::Rect(),  // Empty rect means capture entire surface
      gfx::Size(),  // Empty size means use surface's natural size
      base::BindOnce(&FastVLMTabContentExtractor::OnSurfaceCopied, web_contents,
                     tab_index, std::move(callback)));
}

// static
void FastVLMTabContentExtractor::OnSurfaceCopied(
    content::WebContents* web_contents,
    int tab_index,
    ExtractContentCallback callback,
    const SkBitmap& bitmap) {
  if (bitmap.empty()) {
    LOG(ERROR) << "Failed to capture surface for tab " << tab_index
               << " - empty bitmap";
    std::move(callback).Run(tab_index, "");
    return;
  }

  LOG(INFO) << "Successfully captured surface for tab " << tab_index
            << " - size: " << bitmap.width() << "x" << bitmap.height();

  // Encode the bitmap to PNG format for FastVLM
  auto png_data_opt = gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false);
  if (!png_data_opt.has_value()) {
    LOG(ERROR) << "Failed to encode bitmap to PNG for tab " << tab_index;
    std::move(callback).Run(tab_index, "");
    return;
  }

  // Convert to vector<uint8_t> for FastVLM service
  std::vector<uint8_t> image_data = std::move(png_data_opt.value());

  LOG(INFO) << "Encoded surface to PNG for tab " << tab_index
            << " - size: " << image_data.size() << " bytes";

  // Get FastVLM service from the browser context
  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  auto* fast_vlm_service =
      FastVLMServiceFactory::GetForBrowserContext(browser_context);

  if (!fast_vlm_service) {
    LOG(ERROR) << "FastVLM service not available for tab " << tab_index;
    std::move(callback).Run(tab_index, "");
    return;
  }

  // Let RunInference handle initialization and readiness checks

  // Create a prompt for extracting page content description
  std::string analysis_prompt =
      "Analyze this webpage screenshot and provide a concise description of "
      "the main content, topics, and key information visible. Focus on "
      "the primary subject matter and any text or visual elements that "
      "indicate what this page is about. Keep it under 200 words.";

  LOG(INFO) << "Running FastVLM analysis for tab " << tab_index << " with "
            << image_data.size() << " byte image";

  // Run FastVLM inference on the screenshot
  fast_vlm_service->RunInference(
      image_data, analysis_prompt,
      256,  // max_tokens - reduced with better stopping conditions
      base::BindOnce(&FastVLMTabContentExtractor::OnFastVLMAnalysisComplete,
                     tab_index, std::move(callback)));
}

// static
void FastVLMTabContentExtractor::OnFastVLMAnalysisComplete(
    int tab_index,
    ExtractContentCallback callback,
    bool success,
    const std::string& description) {
  if (!success) {
    LOG(ERROR) << "FastVLM analysis failed for tab " << tab_index;
    std::move(callback).Run(tab_index, "");
    return;
  }

  LOG(INFO) << "FastVLM analysis completed for tab " << tab_index
            << " - description length: " << description.length();

  LOG(ERROR) << description;

  // Return the AI-generated visual description
  std::move(callback).Run(tab_index, description);
}

}  // namespace local_ai

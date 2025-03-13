// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/brave_screenshots_tab_feature.h"

#include "base/notreached.h"
#include "brave/browser/brave_screenshots/screenshots_utils.h"
#include "brave/browser/brave_screenshots/strategies/fullpage_strategy.h"
#include "brave/browser/brave_screenshots/strategies/selection_strategy.h"
#include "brave/browser/brave_screenshots/strategies/viewport_strategy.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "content/public/browser/web_contents.h"

namespace {

// Some screenshots may need to be clipped to avoid the GPU limit.
// See https://crbug.com/1260828. When this happens, we may wish to notify the
// user that only a portion of their page could be captured.
void DisplayScreenshotClippedNotification(
    base::WeakPtr<content::WebContents> web_contents) {
  // Issue: https://github.com/brave/brave-browser/issues/43369
  NOTIMPLEMENTED();
}

}  // namespace
namespace brave_screenshots {

BraveScreenshotsTabFeature::BraveScreenshotsTabFeature(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
  DVLOG(1) << "BraveScreenshotsTabFeature created";
}

BraveScreenshotsTabFeature::~BraveScreenshotsTabFeature() {
  DVLOG(1) << "BraveScreenshotsTabFeature destroyed";
}

void BraveScreenshotsTabFeature::StartScreenshot(ScreenshotType type) {
  DVLOG(1) << "Called StartScreenshot";

  // We've determined the appropriate strategy to use
  strategy_ = CreateStrategy(type);

  DVLOG(2) << "Starting capture";

  strategy_->Capture(
      web_contents_,
      base::BindOnce(&BraveScreenshotsTabFeature::OnCaptureComplete,
                     weak_factory_.GetWeakPtr()));
}

std::unique_ptr<BraveScreenshotStrategy>
BraveScreenshotsTabFeature::CreateStrategy(ScreenshotType type) {
  switch (type) {
    case ScreenshotType::kFullPage:
      DVLOG(3) << "Creating FullPageStrategy";
      return std::make_unique<FullPageStrategy>();
    case ScreenshotType::kSelection:
      // Based on image_editor::ScreenshotFlow, which requires a WebContents
      DVLOG(3) << "Creating SelectionStrategy";
      return std::make_unique<SelectionStrategy>(web_contents_);
    case ScreenshotType::kViewport:
      // Based on image_editor::ScreenshotFlow, which requires a WebContents
      DVLOG(3) << "Creating ViewportStrategy";
      return std::make_unique<ViewportStrategy>(web_contents_);
    default:
      NOTREACHED();
  }
}

bool BraveScreenshotsTabFeature::IsScreenshotInProgress() const {
  return strategy_ != nullptr;
}

void BraveScreenshotsTabFeature::OnCaptureComplete(
    const image_editor::ScreenshotCaptureResult& result) {
  DVLOG(2) << __func__;

  if (result.image.IsEmpty()) {
    DVLOG(2) << "Screenshot capture failed";
    strategy_.reset();
    return;
  }

  if (strategy_->DidClipScreenshot()) {
    DisplayScreenshotClippedNotification(web_contents_->GetWeakPtr());
  }

  utils::CopyImageToClipboard(result);
  utils::DisplayScreenshotBubble(result, web_contents_->GetWeakPtr());

  strategy_.reset();
}

}  // namespace brave_screenshots

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/screenshots_tab_feature.h"

#include "base/notreached.h"
#include "brave/browser/brave_screenshots/screenshots_utils.h"
#include "brave/browser/brave_screenshots/strategies/fullpage_strategy.h"
#include "brave/browser/brave_screenshots/strategies/selection_strategy.h"
#include "brave/browser/brave_screenshots/strategies/viewport_strategy.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_contents.h"

namespace {

// Some screenshots may need to be clipped to avoid the GPU limit.
// See https://crbug.com/1260828. When this happens, we may wish to notify the
// user that only a portion of their page could be captured.
void DisplayScreenshotClippedNotification(base::WeakPtr<Browser> browser) {
  NOTIMPLEMENTED();
}

}  // namespace
namespace brave_screenshots {

BraveScreenshotsTabFeature::BraveScreenshotsTabFeature() {
  DVLOG(1) << "BraveScreenshotsTabFeature created";
}

BraveScreenshotsTabFeature::~BraveScreenshotsTabFeature() {
  DVLOG(1) << "BraveScreenshotsTabFeature destroyed";
  if (strategy_) {
    strategy_.reset();
  }
}

void BraveScreenshotsTabFeature::StartScreenshot(Browser* browser,
                                                 ScreenshotType type) {
  DVLOG(1) << "Starting screenshot capture";
  CHECK(browser);

  browser_ = browser->AsWeakPtr();
  web_contents_ =
      browser_->tab_strip_model()->GetActiveWebContents()->GetWeakPtr();

  // Instantiate the strategy and start the capture
  if (strategy_) {
    strategy_.reset();
  }

  // We've determined the appropriate strategy to use
  strategy_ = CreateStrategy(type);

  if (!strategy_) {
    OnCaptureComplete({});
    return;
  }

  DVLOG(2) << "Starting capture";

  strategy_->Capture(
      web_contents_.get(),
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
      return std::make_unique<SelectionStrategy>(web_contents_.get());
    case ScreenshotType::kViewport:
      // Based on image_editor::ScreenshotFlow, which requires a WebContents
      DVLOG(3) << "Creating ViewportStrategy";
      return std::make_unique<ViewportStrategy>(web_contents_.get());
    default:
      NOTREACHED();
  }
}

void BraveScreenshotsTabFeature::OnCaptureComplete(
    const image_editor::ScreenshotCaptureResult& result) {
  DVLOG(2) << __func__;

  if (result.image.IsEmpty()) {
    DVLOG(2) << "Screenshot capture failed";
    return;
  }

  if (strategy_->DidClipScreenshot()) {
    DisplayScreenshotClippedNotification(browser_);
  }

  if (browser_) {
    utils::CopyImageToClipboard(result);
    utils::DisplayScreenshotBubble(result, browser_);
  }
}

}  // namespace brave_screenshots

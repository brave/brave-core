// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_screenshots/screenshots_tab_feature.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_screenshots/devtools_helper.h"
#include "brave/browser/brave_screenshots/screenshots_utils.h"
#include "chrome/browser/image_editor/screenshot_flow.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/web_contents.h"

namespace brave_screenshots {

BraveScreenshotsTabFeature::BraveScreenshotsTabFeature(
    content::WebContents* web_contents)
    : image_editor::ScreenshotFlow(web_contents) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

BraveScreenshotsTabFeature::~BraveScreenshotsTabFeature() = default;

void BraveScreenshotsTabFeature::StartScreenshot(Browser* browser,
                                                 ScreenshotType type) {
  if (!browser) {
    return;
  }

  browser_ = browser->AsWeakPtr();

  image_editor::ScreenshotCaptureCallback callback = base::BindOnce(
      &BraveScreenshotsTabFeature::OnCaptureComplete, weak_this_);

  switch (type) {
    case ScreenshotType::kSelection:
      image_editor::ScreenshotFlow::Start(std::move(callback));
      break;
    case ScreenshotType::kViewport:
      image_editor::ScreenshotFlow::StartFullscreenCapture(std::move(callback));
      break;
    case ScreenshotType::kFullPage: {
      // Full page screenshots require DevTools
      if (!InitDevToolsHelper(std::move(callback))) {
        return;
      }

      SendCaptureFullscreenCommand();
      break;
    }
    default:
      NOTREACHED();
  }
}

bool BraveScreenshotsTabFeature::InitDevToolsHelper(
    image_editor::ScreenshotCaptureCallback callback) {
  devtools_helper_ = std::make_unique<DevToolsHelper>(
      web_contents()->GetWeakPtr(), std::move(callback));

  return devtools_helper_->Attach();
}

void BraveScreenshotsTabFeature::SendCaptureFullscreenCommand() {
  if (!devtools_helper_) {
    return;
  }
  devtools_helper_->SendCaptureFullscreenCommand();
}

void BraveScreenshotsTabFeature::OnCaptureComplete(
    const image_editor::ScreenshotCaptureResult& result) {
  utils::CopyImageToClipboard(result);
  utils::DisplayScreenshotBubble(result, browser_);
}

}  // namespace brave_screenshots

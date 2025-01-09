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

void DisplayScreenshotClippedNotification(base::WeakPtr<Browser> browser) {
  NOTIMPLEMENTED();
  // message_center::RichNotificationData notification_data;

  // const std::unique_ptr<message_center::Notification> notification =
  //     std::make_unique<message_center::Notification>(
  //         message_center::NOTIFICATION_TYPE_SIMPLE,
  //         "brave_screenshots_clipped", u"Clipped", u"Sorries",
  //         ui::ImageModel(), std::u16string(), GURL(),
  //         message_center::NotifierId(
  //             message_center::NotifierType::SYSTEM_COMPONENT,
  //             "feature.brave_screenshots"),
  //         notification_data, nullptr);

  // // Note: Notifications cannot be both TRANSIENT and have a nullptr delegate
  // NotificationDisplayServiceFactory::GetForProfile(browser->GetProfile())
  //     ->Display(NotificationHandler::Type::ANNOUNCEMENT, *notification,
  //               nullptr);
}

}  // namespace
namespace brave_screenshots {

BraveScreenshotsTabFeature::BraveScreenshotsTabFeature() {
  VLOG(1) << "BraveScreenshotsTabFeature created";
}

BraveScreenshotsTabFeature::~BraveScreenshotsTabFeature() {
  VLOG(1) << "BraveScreenshotsTabFeature destroyed";
  if (strategy_) {
    strategy_.reset();
  }
}

void BraveScreenshotsTabFeature::StartScreenshot(Browser* browser,
                                                 ScreenshotType type) {
  VLOG(1) << "Starting screenshot capture";
  CHECK(browser);
  browser_ = browser->AsWeakPtr();
  web_contents_ =
      browser_->tab_strip_model()->GetActiveWebContents()->GetWeakPtr();

  // Instantiate the strategy and start the capture
  if (strategy_) {
    strategy_.reset();
  }

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
      VLOG(3) << "Creating FullPageStrategy";
      return std::make_unique<FullPageStrategy>();
    case ScreenshotType::kSelection:
      // Based on image_editor::ScreenshotFlow, which requires a WebContents
      VLOG(3) << "Creating SelectionStrategy";
      return std::make_unique<SelectionStrategy>(web_contents_.get());
    case ScreenshotType::kViewport:
      // Based on image_editor::ScreenshotFlow, which requires a WebContents
      VLOG(3) << "Creating ViewportStrategy";
      return std::make_unique<ViewportStrategy>(web_contents_.get());
    default:
      NOTREACHED();
  }
}

void BraveScreenshotsTabFeature::OnCaptureComplete(
    const image_editor::ScreenshotCaptureResult& result) {
  DVLOG(2) << __func__;
  if (result.image.IsEmpty()) {
    LOG(ERROR) << "Screenshot capture failed";
    return;
  }

  if (strategy_->DidClipScreenshot()) {
    DisplayScreenshotClippedNotification(browser_);
  }

  // While the image will be written to the clipboard, depending on its size it
  // may not be displayed within Windows' clipboard history (Win+V)
  utils::CopyImageToClipboard(result);
  utils::DisplayScreenshotBubble(result, browser_);
}

}  // namespace brave_screenshots

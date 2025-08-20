/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.h"

#include "chrome/browser/download/bubble/download_bubble_update_service.h"
#include "chrome/browser/download/bubble/download_bubble_update_service_factory.h"
#include "chrome/browser/download/download_ui_model.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/models/image_model.h"

namespace {

SkColor GetIconColor(SkColor chromium_color,
                     DownloadDisplay::IconState state,
                     DownloadDisplay::IconActive active,
                     const ui::ColorProvider* color_provider) {
  // Apply active color only when download is completed and user doesn't
  // interact with this button.
  if (state == DownloadDisplay::IconState::kComplete &&
      active == DownloadDisplay::IconActive::kActive) {
    return color_provider->GetColor(kColorDownloadToolbarButtonActive);
  }

  // Otherwise, always use inactive color.
  return color_provider->GetColor(kColorDownloadToolbarButtonInactive);
}

}  // namespace

#define DownloadToolbarUIController DownloadToolbarUIController_ChromiumImpl

// Update upstream color with our own. Pass COLOR in so that icon_color variable
// in the original file isn't flagged as unused by the compiler.
#define FromVectorIcon(ICON, COLOR)                         \
  FromVectorIcon(ICON, GetIconColor(COLOR, state_, active_, \
                                    browser_view_->GetColorProvider()))

#include <chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.cc>
#undef FromVectorIcon
#undef DownloadToolbarUIController

void DownloadToolbarUIController::UpdateIcon() {
  DownloadToolbarUIController_ChromiumImpl::UpdateIcon();

  if (!action_item_.get()) {
    return;
  }

  auto* button = GetDownloadsButton(browser_view_);
  if (!button) {
    return;
  }

  // Use an exclamation point icon while there's an insecure download in the
  // download models.
  if (HasInsecureDownloads()) {
    auto icon_color = browser_view_->GetColorProvider()->GetColor(
        ui::kColorAlertMediumSeverityIcon);
    button->SetIconEnabledColorsOverride(icon_color);
    button->SetVectorIcon(vector_icons::kNotSecureWarningIcon);
    const gfx::VectorIcon* new_icon = &vector_icons::kNotSecureWarningIcon;
    const int icon_size = action_item_->GetImage().Size().height();
    action_item_->SetImage(
        ui::ImageModel::FromVectorIcon(*new_icon, icon_color, icon_size));
  } else {
    button->SetIconEnabledColorsOverride(std::nullopt);
  }
}

bool DownloadToolbarUIController::HasInsecureDownloads() {
  auto* update_service = DownloadBubbleUpdateServiceFactory::GetForProfile(
      browser_view_->GetProfile());
  if (!update_service || !update_service->IsInitialized()) {
    return false;
  }

  std::vector<DownloadUIModel::DownloadUIModelPtr> all_models;
  update_service->GetAllModelsToDisplay(all_models, /*web_app_id=*/nullptr,
                                        /*force_backfill_download_items=*/true);

  return std::ranges::any_of(all_models, [](const auto& model) {
    return (model->GetInsecureDownloadStatus() ==
                download::DownloadItem::InsecureDownloadStatus::BLOCK ||
            model->GetInsecureDownloadStatus() ==
                download::DownloadItem::InsecureDownloadStatus::WARN);
  });
}

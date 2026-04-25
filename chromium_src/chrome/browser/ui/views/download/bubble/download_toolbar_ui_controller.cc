/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.h"

#include "chrome/browser/download/bubble/download_bubble_update_service.h"
#include "chrome/browser/download/bubble/download_bubble_update_service_factory.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/vector_icons/vector_icons.h"

namespace {

SkColor GetIconColor(DownloadDisplay::IconState state,
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

bool HasInsecureDownloads(BrowserView* browser_view) {
  auto* update_service = DownloadBubbleUpdateServiceFactory::GetForProfile(
      browser_view->GetProfile());
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

// Forward-declaring this customisation point as it depends on
// `GetDownloadsButton`, which is declared in the unamed namespace of the
// shadowed source.
void UpdateIcon_BraveImpl(BrowserView* browser_view,
                          actions::ActionItem* action_item);

}  // namespace

#include <chrome/browser/ui/views/download/bubble/download_toolbar_ui_controller.cc>

namespace {

void UpdateIcon_BraveImpl(BrowserView* browser_view,
                          actions::ActionItem* action_item) {
  if (!action_item) {
    return;
  }

  auto* button = GetDownloadsButton(browser_view);
  if (!button) {
    return;
  }

  // Use an exclamation point icon while there's an insecure download in the
  // download models.
  if (HasInsecureDownloads(browser_view)) {
    auto icon_color = browser_view->GetColorProvider()->GetColor(
        ui::kColorAlertMediumSeverityIcon);
    button->SetIconEnabledColorsOverride(icon_color);
    button->SetVectorIcon(vector_icons::kNotSecureWarningIcon);
    const gfx::VectorIcon* new_icon = &vector_icons::kNotSecureWarningIcon;
    const int icon_size = action_item->GetImage().Size().height();
    action_item->SetImage(
        ui::ImageModel::FromVectorIcon(*new_icon, icon_color, icon_size));
  } else {
    button->SetIconEnabledColorsOverride(std::nullopt);
  }
}

}  // namespace

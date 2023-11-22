/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/bubble/download_toolbar_button_view.h"

#include "base/ranges/algorithm.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace gfx {

SkRect AdjustRingBounds(const gfx::RectF& ring_bounds);

}  // namespace gfx

#define RectFToSkRect(ring_bounds) AdjustRingBounds(ring_bounds)
#define DownloadToolbarButtonView DownloadToolbarButtonViewChromium
#define FromVectorIcon(icon, color) FromVectorIcon(icon, color, 16)

#include "src/chrome/browser/ui/views/download/bubble/download_toolbar_button_view.cc"

#undef FromVectorIcon
#undef DownloadToolbarButtonView
#undef RectFToSkRect

namespace gfx {

SkRect AdjustRingBounds(const gfx::RectF& ring_bounds) {
  const int chromium_ring_radius = ui::TouchUiController::Get()->touch_ui()
                                       ? kProgressRingRadiusTouchMode
                                       : kProgressRingRadius;
  constexpr auto kBraveRingRadius = 12;
  const auto delta = kBraveRingRadius - chromium_ring_radius;
  gfx::RectF new_bounds(ring_bounds);
  new_bounds.Outset(gfx::OutsetsF(delta));
  return gfx::RectFToSkRect(new_bounds);
}

}  // namespace gfx

SkColor DownloadToolbarButtonView::GetIconColor() const {
  // Apply active color only when download is completed and user doesn't
  // interact with this button.
  if (GetIconState() == DownloadDisplay::IconState::kComplete &&
      active_ == DownloadDisplay::IconActive::kActive) {
    return GetColorProvider()->GetColor(kColorBraveDownloadToolbarButtonActive);
  }

  // Otherwise, always use inactive color.
  return GetColorProvider()->GetColor(kColorDownloadToolbarButtonInactive);
}

void DownloadToolbarButtonView::PaintButtonContents(gfx::Canvas* canvas) {
  // Don't draw anything but alert icon when insecure download is in-progress.
  if (HasInsecureDownloads()) {
    return;
  }

  DownloadToolbarButtonViewChromium::PaintButtonContents(canvas);
}

void DownloadToolbarButtonView::UpdateIcon() {
  if (!GetWidget()) {
    return;
  }

  DownloadToolbarButtonViewChromium::UpdateIcon();

  // Replace Icon when insecure download is in-progress.
  if (HasInsecureDownloads()) {
    const gfx::VectorIcon* new_icon = &vector_icons::kNotSecureWarningIcon;
    SkColor icon_color =
        GetColorProvider()->GetColor(ui::kColorAlertMediumSeverityIcon);

    constexpr int kIconSize = 16;
    SetImageModel(
        ButtonState::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(*new_icon, icon_color, kIconSize));
    SetImageModel(
        ButtonState::STATE_HOVERED,
        ui::ImageModel::FromVectorIcon(*new_icon, icon_color, kIconSize));
    SetImageModel(
        ButtonState::STATE_PRESSED,
        ui::ImageModel::FromVectorIcon(*new_icon, icon_color, kIconSize));
    SetImageModel(
        Button::STATE_DISABLED,
        ui::ImageModel::FromVectorIcon(
            *new_icon, GetForegroundColor(ButtonState::STATE_DISABLED),
            kIconSize));
  }
}

bool DownloadToolbarButtonView::HasInsecureDownloads() {
  auto* update_service = bubble_controller()->update_service();
  if (!update_service || !update_service->IsInitialized()) {
    return false;
  }

  std::vector<DownloadUIModel::DownloadUIModelPtr> all_models;
  update_service->GetAllModelsToDisplay(all_models, /*web_app_id=*/nullptr,
                                        /*force_backfill_download_items=*/true);

  return base::ranges::any_of(all_models, [](const auto& model) {
    return (model->GetInsecureDownloadStatus() ==
                download::DownloadItem::InsecureDownloadStatus::BLOCK ||
            model->GetInsecureDownloadStatus() ==
                download::DownloadItem::InsecureDownloadStatus::WARN);
  });
}

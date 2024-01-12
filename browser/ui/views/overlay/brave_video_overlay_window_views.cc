/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

#include <vector>

#include "brave/browser/ui/views/overlay/brave_back_to_tab_label_button.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/overlay/back_to_tab_label_button.h"
#include "chrome/browser/ui/views/overlay/close_image_button.h"
#include "chrome/browser/ui/views/overlay/constants.h"
#include "chrome/browser/ui/views/overlay/hang_up_button.h"
#include "chrome/browser/ui/views/overlay/playback_image_button.h"
#include "chrome/browser/ui/views/overlay/simple_overlay_window_image_button.h"
#include "chrome/browser/ui/views/overlay/toggle_camera_button.h"
#include "chrome/browser/ui/views/overlay/toggle_microphone_button.h"

namespace {

constexpr int kTopControlIconSize = 20;

}  // namespace

BraveVideoOverlayWindowViews::BraveVideoOverlayWindowViews(
    content::VideoPictureInPictureWindowController* controller)
    : VideoOverlayWindowViews(controller) {}

void BraveVideoOverlayWindowViews::SetUpViews() {
  VideoOverlayWindowViews::SetUpViews();
  UpdateControlIcons();
}

void BraveVideoOverlayWindowViews::OnUpdateControlsBounds() {
  VideoOverlayWindowViews::OnUpdateControlsBounds();

  const auto& window_size = GetBounds().size();

  // Lay out controls on top of window
  constexpr int kTopControlSpacing = 16;
  const auto close_button_insets = close_controls_view_->GetInsets();
  const gfx::Size close_button_size(
      kTopControlIconSize + close_button_insets.width(),
      kTopControlIconSize + close_button_insets.height());
  // Upcasting in order to call a base class's SetPosition() which is hidden by
  // the CloseImageButton::SetPosition().
  close_controls_view_->SetSize(close_button_size);
  views::AsViewClass<OverlayWindowImageButton>(close_controls_view_.get())
      ->SetPosition({window_size.width() -
                         (kTopControlSpacing - close_button_insets.right()) -
                         close_controls_view_->width(),
                     kTopControlSpacing - close_button_insets.top()});

  if (back_to_tab_label_button_) {
    back_to_tab_label_button_->SetMinSize(close_button_size);
    back_to_tab_label_button_->SetMaxSize(close_button_size);
    back_to_tab_label_button_->SetSize(close_button_size);
    back_to_tab_label_button_->SetPosition(
        {close_controls_view_->origin().x() -
             (kTopControlSpacing - close_button_insets.left() -
              back_to_tab_label_button_->GetInsets().right()) -
             back_to_tab_label_button_->size().width(),
         close_controls_view_->origin().y()});
  }

  // Lay out controls in the middle of window
  std::vector<views::View*> visible_controls;
  if (previous_track_controls_view_->GetVisible()) {
    visible_controls.push_back(previous_track_controls_view_.get());
  }
  if (play_pause_controls_view_->GetVisible()) {
    visible_controls.push_back(play_pause_controls_view_.get());
  }
  if (next_track_controls_view_->GetVisible()) {
    visible_controls.push_back(next_track_controls_view_.get());
  }
  if (previous_slide_controls_view_->GetVisible()) {
    visible_controls.push_back(previous_slide_controls_view_.get());
  }
  if (next_slide_controls_view_->GetVisible()) {
    visible_controls.push_back(next_slide_controls_view_.get());
  }
  if (toggle_microphone_button_->GetVisible()) {
    visible_controls.push_back(toggle_microphone_button_.get());
  }
  if (toggle_camera_button_->GetVisible()) {
    visible_controls.push_back(toggle_camera_button_.get());
  }
  if (hang_up_button_->GetVisible()) {
    visible_controls.push_back(hang_up_button_.get());
  }

  constexpr int kCenterControlIconSize = 32;
  constexpr int kCenterControlSize =
      kCenterControlIconSize + kPipWindowIconPadding * 2;
  constexpr int kCenterControlSpacing = 24 - kPipWindowIconPadding * 2;

  const auto visible_controls_size = visible_controls.size();
  int x = (window_size.width() - visible_controls_size * kCenterControlSize -
           kCenterControlSpacing * (visible_controls_size - 1)) /
          2;
  const int y = (window_size.height() - kCenterControlSize) / 2;
  for (size_t i = 0; i < visible_controls_size; i++) {
    visible_controls[i]->SetBounds(x, y, kCenterControlSize,
                                   kCenterControlSize);
    x += kCenterControlSize + kCenterControlSpacing;
  }
}

void BraveVideoOverlayWindowViews::UpdateControlIcons() {
  close_controls_view_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoCloseIcon, kColorPipWindowForeground,
                                     kTopControlIconSize));

  if (back_to_tab_label_button_) {
    back_to_tab_label_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(kLeoPictureInPictureReturnIcon,
                                       kColorPipWindowForeground,
                                       kTopControlIconSize));
    back_to_tab_label_button_->SetText({});
  }

  previous_track_controls_view_->override_icon(kLeoPreviousOutlineIcon);
  next_track_controls_view_->override_icon(kLeoNextOutlineIcon);
}

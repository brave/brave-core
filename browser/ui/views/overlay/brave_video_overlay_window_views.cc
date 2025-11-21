/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

#include <initializer_list>

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/overlay/back_to_tab_button.h"
#include "chrome/browser/ui/views/overlay/back_to_tab_label_button.h"
#include "chrome/browser/ui/views/overlay/close_image_button.h"
#include "chrome/browser/ui/views/overlay/constants.h"
#include "chrome/browser/ui/views/overlay/hang_up_button.h"
#include "chrome/browser/ui/views/overlay/minimize_button.h"
#include "chrome/browser/ui/views/overlay/playback_image_button.h"
#include "chrome/browser/ui/views/overlay/simple_overlay_window_image_button.h"
#include "chrome/browser/ui/views/overlay/toggle_camera_button.h"
#include "chrome/browser/ui/views/overlay/toggle_microphone_button.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view_utils.h"

namespace {

constexpr int kTopControlIconSize = 20;

}  // namespace

BraveVideoOverlayWindowViews::BraveVideoOverlayWindowViews(
    content::VideoPictureInPictureWindowController* controller)
    : VideoOverlayWindowViews(controller) {
}

BraveVideoOverlayWindowViews::~BraveVideoOverlayWindowViews() = default;

void BraveVideoOverlayWindowViews::SetUpViews() {
  VideoOverlayWindowViews::SetUpViews();
  UpdateControlIcons();
}

void BraveVideoOverlayWindowViews::OnUpdateControlsBounds() {
  VideoOverlayWindowViews::OnUpdateControlsBounds();

  LayoutTopControls();
  LayoutCenterControls();
}

void BraveVideoOverlayWindowViews::UpdateControlIcons() {
  close_controls_view_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoCloseIcon, kColorPipWindowForeground,
                                     kTopControlIconSize));

  if (minimize_button_) {
    minimize_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(kChromiumMinimizeIcon,
                                       kColorPipWindowForeground,
                                       kTopControlIconSize));
  }

  previous_track_controls_view_->override_icon(kLeoPreviousOutlineIcon);
  next_track_controls_view_->override_icon(kLeoNextOutlineIcon);
}

void BraveVideoOverlayWindowViews::LayoutTopControls() {
  // Copies size in order to avoid ASAN failure
  // https://github.com/brave/internal/issues/1108
  const auto window_size = GetBounds().size();

  // Lay out controls on top of window
  constexpr int kTopControlSpacing = 16;
  auto get_new_size = [](const gfx::Insets& insets) {
    return gfx::Size(kTopControlIconSize + insets.width(),
                     kTopControlIconSize + insets.height());
  };

  // Lay out controls from right to left. As we'd like to apply spacing based
  // on the "Icon" size, we take away insets when calculating spacing.
  views::View* previous_view = nullptr;
  for (auto* view : std::initializer_list<views::View*>{
           close_controls_view_.get(), back_to_tab_label_button_.get(),
           minimize_button_.get()}) {
    if (!view || view->GetVisible()) {
      continue;
    }

    auto insets = view->GetInsets();
    close_controls_view_->SetSize(get_new_size(insets));

    const int x =
        previous_view
            ? previous_view->origin().x() -
                  (kTopControlSpacing - previous_view->GetInsets().left() -
                   insets.right()) -
                  view->size().width()
            : window_size.width() - (kTopControlSpacing - insets.right()) -
                  view->size().width();
    const int y = kTopControlSpacing - insets.top();
    // Note that we're usin View::SetPosition() here instead of
    // CloseImageButton::SetPosition() and BackToTabButton::SetPosition().
    view->SetPosition({x, y});

    previous_view = view;
  }
}

void BraveVideoOverlayWindowViews::LayoutCenterControls() {
  // Lay out controls in the middle of window
  const auto window_size = GetBounds().size();

  std::vector<OverlayWindowImageButton*> visible_controls;
  if (replay_10_seconds_button_->GetVisible()) {
    visible_controls.push_back(replay_10_seconds_button_.get());
  }
  if (play_pause_controls_view_->GetVisible()) {
    visible_controls.push_back(play_pause_controls_view_.get());
  }
  if (forward_10_seconds_button_->GetVisible()) {
    visible_controls.push_back(forward_10_seconds_button_.get());
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
    if (views::IsViewClass<SimpleOverlayWindowImageButton>(
            visible_controls[i])) {
      views::AsViewClass<SimpleOverlayWindowImageButton>(visible_controls[i])
          ->set_icon_size(kCenterControlIconSize);
    }
    visible_controls[i]->SetBounds(x, y, kCenterControlSize,
                                   kCenterControlSize);
    x += kCenterControlSize + kCenterControlSpacing;
  }
}

void BraveVideoOverlayWindowViews::OnKeyEvent(ui::KeyEvent* event) {
  if (event->type() == ui::EventType::kKeyPressed) {
    if (event->key_code() == ui::VKEY_LEFT) {
      Replay10Seconds();
      event->SetHandled();
      return;
    }
    if (event->key_code() == ui::VKEY_RIGHT) {
      Forward10Seconds();
      event->SetHandled();
      return;
    }
  }

  VideoOverlayWindowViews::OnKeyEvent(event);
}


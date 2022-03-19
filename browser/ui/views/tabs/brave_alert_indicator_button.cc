/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_alert_indicator_button.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "brave/common/brave_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_types.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkPathTypes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/background.h"

namespace {

bool IsAudioState(const absl::optional<TabAlertState>& state) {
  return (state.has_value() && (state.value() == TabAlertState::AUDIO_PLAYING ||
                                state.value() == TabAlertState::AUDIO_MUTING));
}

}  // namespace

class BraveAlertIndicatorButton::BraveAlertBackground
    : public views::Background {
 public:
  explicit BraveAlertBackground(BraveAlertIndicatorButton* host_view)
      : host_view_(host_view) {}

  BraveAlertBackground(const BraveAlertBackground&) = delete;
  BraveAlertBackground& operator=(const BraveAlertBackground&) = delete;

  // views::Background overrides:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    if (!host_view_->IsTabAudioToggleable())
      return;

    gfx::Point center = host_view_->GetContentsBounds().CenterPoint();
    SkPath path;
    path.setFillType(SkPathFillType::kEvenOdd);
    path.addCircle(center.x(), center.y(), host_view_->width() / 2);
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(host_view_->GetBackgroundColor());
    canvas->DrawPath(path, flags);
  }

 private:
  raw_ptr<BraveAlertIndicatorButton> host_view_ = nullptr;
};

BraveAlertIndicatorButton::BraveAlertIndicatorButton(Tab* parent_tab)
    : AlertIndicatorButton(parent_tab) {
  SetBackground(std::make_unique<BraveAlertBackground>(this));
}

SkColor BraveAlertIndicatorButton::GetBackgroundColor() const {
  SkColor fill_color = parent_tab_->controller()->GetTabBackgroundColor(
      parent_tab_->IsActive() ? TabActive::kInactive : TabActive::kActive,
      BrowserFrameActiveState::kUseCurrent);

  if (!IsTabAudioToggleable() || !IsMouseHovered())
    return fill_color;

  // Approximating the InkDrop behavior of the close button.
  return color_utils::BlendTowardMaxContrast(fill_color,
                                             mouse_pressed_ ? 72 : 36);
}

bool BraveAlertIndicatorButton::OnMousePressed(const ui::MouseEvent& event) {
  mouse_pressed_ = true;
  SchedulePaint();

  if (!IsTabAudioToggleable())
    return AlertIndicatorButton::OnMousePressed(event);

  return true;
}

void BraveAlertIndicatorButton::OnMouseReleased(const ui::MouseEvent& event) {
  mouse_pressed_ = false;
  SchedulePaint();

  if (!IsTabAudioToggleable() || !IsMouseHovered())
    return AlertIndicatorButton::OnMouseReleased(event);

  auto* tab_strip = static_cast<TabStrip*>(parent_tab_->controller());
  const int tab_index = tab_strip->GetModelIndexOf(parent_tab_);
  if (tab_index == -1)
    return;
  auto* tab_strip_model =
      static_cast<BrowserTabStripController*>(tab_strip->controller())->model();
  auto* web_contents = tab_strip_model->GetWebContentsAt(tab_index);
  if (web_contents == nullptr)
    return;
  chrome::SetTabAudioMuted(web_contents, !web_contents->IsAudioMuted(),
                           TabMutedReason::CONTENT_SETTING, std::string());
}

void BraveAlertIndicatorButton::OnMouseEntered(const ui::MouseEvent& event) {
  if (IsTabAudioToggleable())
    SchedulePaint();
  AlertIndicatorButton::OnMouseExited(event);
}

void BraveAlertIndicatorButton::OnMouseExited(const ui::MouseEvent& event) {
  if (IsTabAudioToggleable())
    SchedulePaint();
  AlertIndicatorButton::OnMouseExited(event);
}

bool BraveAlertIndicatorButton::OnMouseDragged(const ui::MouseEvent& event) {
  if (IsTabAudioToggleable())
    SchedulePaint();
  return AlertIndicatorButton::OnMouseDragged(event);
}

bool BraveAlertIndicatorButton::IsTabAudioToggleable() const {
  // The alert indicator being interactive can be disabled entirely
  if (!base::FeatureList::IsEnabled(features::kTabAudioIconInteractive)) {
    return false;
  }

  // Pinned tabs are too small to select if alert indicator is a button
  if (parent_tab_->controller()->IsTabPinned(parent_tab_))
    return false;

  return IsAudioState(alert_state_);
}

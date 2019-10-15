/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_alert_indicator.h"

#include <memory>
#include <string>

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/tabs/browser_tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_controller.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "chrome/browser/ui/views/tabs/tab_style_views.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/background.h"

namespace {

bool IsAudioState(TabAlertState state) {
  return (state == TabAlertState::AUDIO_PLAYING ||
          state == TabAlertState::AUDIO_MUTING);
}

}  // namespace

class BraveAlertIndicator::BraveAlertBackground : public views::Background {
 public:
  explicit BraveAlertBackground(BraveAlertIndicator* host_view)
    : host_view_(host_view) {
  }

  // views::Background overrides:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    gfx::Point center = host_view_->GetContentsBounds().CenterPoint();
    SkPath path;
    path.setFillType(SkPath::kEvenOdd_FillType);
    path.addCircle(center.x(), center.y(), host_view_->width() / 2);
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(host_view_->GetBackgroundColor());
    canvas->DrawPath(path, flags);
  }

 private:
  BraveAlertIndicator* host_view_;

  DISALLOW_COPY_AND_ASSIGN(BraveAlertBackground);
};

BraveAlertIndicator::BraveAlertIndicator(Tab* parent_tab)
    : AlertIndicator(parent_tab) {
  SetBackground(std::make_unique<BraveAlertBackground>(this));
}

SkColor BraveAlertIndicator::GetBackgroundColor() const {
  TabStyle::TabColors colors = parent_tab_->tab_style()->CalculateColors();
  if (!IsAudioState(alert_state_) || !IsMouseHovered())
    return colors.background_color;

  // Approximating the InkDrop behavior of the close button.
  return color_utils::BlendTowardMaxContrast(colors.background_color,
                                             mouse_pressed_ ? 72 : 36);
}

bool BraveAlertIndicator::OnMousePressed(const ui::MouseEvent& event) {
  mouse_pressed_ = true;
  SchedulePaint();

  if (!IsAudioState(alert_state_))
    return AlertIndicator::OnMousePressed(event);

  return true;
}

void BraveAlertIndicator::OnMouseReleased(const ui::MouseEvent& event) {
  mouse_pressed_ = false;
  SchedulePaint();

  if (!IsAudioState(alert_state_) || !IsMouseHovered())
    return AlertIndicator::OnMouseReleased(event);

  auto* tab_strip = static_cast<TabStrip*>(parent_tab_->controller());
  const int tab_index = tab_strip->GetModelIndexOfTab(parent_tab_);
  auto* tab_strip_model = static_cast<BrowserTabStripController*>(
      tab_strip->controller())->model();
  auto* web_contents = tab_strip_model->GetWebContentsAt(tab_index);

  if (!chrome::CanToggleAudioMute(web_contents))
    return AlertIndicator::OnMouseReleased(event);

  chrome::SetTabAudioMuted(web_contents,
                           !web_contents->IsAudioMuted(),
                           TabMutedReason::CONTEXT_MENU,
                           std::string());
}

void BraveAlertIndicator::OnMouseEntered(const ui::MouseEvent& event) {
  if (IsAudioState(alert_state_))
    SchedulePaint();
  AlertIndicator::OnMouseExited(event);
}

void BraveAlertIndicator::OnMouseExited(const ui::MouseEvent& event) {
  if (IsAudioState(alert_state_))
    SchedulePaint();
  AlertIndicator::OnMouseExited(event);
}

bool BraveAlertIndicator::OnMouseDragged(const ui::MouseEvent& event) {
  if (IsAudioState(alert_state_))
    SchedulePaint();
  return AlertIndicator::OnMouseDragged(event);
}

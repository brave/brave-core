/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_ads/notification_view.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"
#include "brave/ui/brave_ads/message_popup_view.h"
#include "brave/ui/brave_ads/notification_background_painter.h"
#include "brave/ui/brave_ads/notification_control_buttons_view.h"
#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "ui/base/win/shell.h"
#endif

namespace brave_ads {

namespace {

static const int kWindowsShadowElevation = 2;
static const int kWindowsShadowRadius = 0;

bool ShouldShowAeroShadowBorder() {
#if defined(OS_WIN)
  return ui::win::IsAeroGlassEnabled();
#else
  return false;
#endif
}

}  // namespace

class NotificationView::HighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  HighlightPathGenerator() = default;

  SkPath GetHighlightPath(const views::View* view) override {
    return static_cast<const NotificationView*>(view)->GetHighlightPath();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HighlightPathGenerator);
};

NotificationView::NotificationView(const Notification& notification) :
    notification_id_(notification.id()),
    slide_out_controller_(this, this) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  views::HighlightPathGenerator::Install(
      this, std::make_unique<HighlightPathGenerator>());

  // Paint to a dedicated layer to make the layer non-opaque.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  UpdateWithNotification(notification);

  UpdateCornerRadius(0, 0);

  // If Aero is enabled, set shadow border.
  if (ShouldShowAeroShadowBorder()) {
    const auto& shadow = gfx::ShadowDetails::Get(
        kWindowsShadowElevation,
        kWindowsShadowRadius);
    gfx::Insets ninebox_insets = gfx::ShadowValue::GetBlurRegion(shadow.values);
    SetBorder(views::CreateBorderPainter(
        views::Painter::CreateImagePainter(shadow.ninebox_image,
                                           ninebox_insets),
        -gfx::ShadowValue::GetMargin(shadow.values)));
  }
}

NotificationView::~NotificationView() {
  RemovedFromWidget();
}

void NotificationView::UpdateWithNotification(
    const Notification& notification) {
  slide_out_controller_.set_slide_mode(CalculateSlideMode());
}

void NotificationView::CloseSwipeControl() {
  slide_out_controller_.CloseSwipeControl();
}

void NotificationView::SlideOutAndClose(int direction) {
  // Do not process events once the message view is animating out.
  // crbug.com/940719
  SetEnabled(false);

  slide_out_controller_.SlideOutAndClose(direction);
}

void NotificationView::UpdateCornerRadius(int top_radius, int bottom_radius) {
  SetCornerRadius(top_radius, bottom_radius);
  SetBackground(views::CreateBackgroundFromPainter(
      std::make_unique<NotificationBackgroundPainter>(top_radius,
                                                      bottom_radius)));
  SchedulePaint();
}

SkPath NotificationView::GetHighlightPath() const {
  gfx::Rect rect(GetBoundsInScreen().size());
  // Shrink focus ring size by -kFocusHaloInset on each side to draw
  // them on top of the notifications. We need to do this because TrayBubbleView
  // has a layer that masks to bounds due to which the focus ring can not extend
  // outside the view.
  int inset = -views::PlatformStyle::kFocusHaloInset;
  rect.Inset(gfx::Insets(inset));

  int top_radius = std::max(0, top_radius_ - inset);
  int bottom_radius = std::max(0, bottom_radius_ - inset);
  SkScalar radii[8] = {top_radius,    top_radius,      // top-left
                       top_radius,    top_radius,      // top-right
                       bottom_radius, bottom_radius,   // bottom-right
                       bottom_radius, bottom_radius};  // bottom-left

  return SkPath().addRoundRect(gfx::RectToSkRect(rect), radii);
}

void NotificationView::OnContainerAnimationStarted() {
  // Not implemented by default.
}

void NotificationView::OnContainerAnimationEnded() {
  // Not implemented by default.
}

void NotificationView::OnPaint(gfx::Canvas* canvas) {
  if (ShouldShowAeroShadowBorder()) {
    // If the border is shadow, paint border first.
    OnPaintBorder(canvas);
    // Clip at the border so we don't paint over it.
    canvas->ClipRect(GetContentsBounds());
    OnPaintBackground(canvas);
  } else {
    views::View::OnPaint(canvas);
  }
}

void NotificationView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_TAP_DOWN: {
      SetDrawBackgroundAsActive(true);
      break;
    }
    case ui::ET_GESTURE_TAP_CANCEL:
    case ui::ET_GESTURE_END: {
      SetDrawBackgroundAsActive(false);
      break;
    }
    case ui::ET_GESTURE_TAP: {
      SetDrawBackgroundAsActive(false);
      event->SetHandled();
      return;
    }
    default: {
      // Do nothing
    }
  }

  if (!event->IsScrollGestureEvent() && !event->IsFlingScrollEvent())
    return;

  if (scroller_)
    scroller_->OnGestureEvent(event);
  event->SetHandled();
}

void NotificationView::RemovedFromWidget() {
  if (!focus_manager_)
    return;
  focus_manager_->RemoveFocusChangeListener(this);
  focus_manager_ = nullptr;
}

void NotificationView::AddedToWidget() {
  focus_manager_ = GetFocusManager();
  if (focus_manager_)
    focus_manager_->AddFocusChangeListener(this);
}

void NotificationView::OnThemeChanged() {
  InkDropHostView::OnThemeChanged();
}

ui::Layer* NotificationView::GetSlideOutLayer() {
  return GetWidget()->GetLayer();
}

void NotificationView::OnSlideStarted() {
  for (auto& observer : observers_) {
    observer.OnSlideStarted(notification_id_);
  }
}

void NotificationView::OnSlideChanged(bool in_progress) {
  for (auto& observer : observers_) {
    observer.OnSlideChanged(notification_id_);
  }
}

void NotificationView::AddObserver(NotificationView::Observer* observer) {
  observers_.AddObserver(observer);
}

void NotificationView::RemoveObserver(NotificationView::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void NotificationView::OnSlideOut() {
  // The notification will be deleted after slide out, so give observers a
  // chance to handle the notification before fulling sliding out.
  for (auto& observer : observers_)
    observer.OnPreSlideOut(notification_id_);

  for (auto& observer : observers_)
    observer.OnSlideOut(notification_id_);
}

void NotificationView::OnWillChangeFocus(
    views::View* before, views::View* now) {}

void NotificationView::OnDidChangeFocus(
    views::View* before, views::View* now) {
  if (Contains(before) || Contains(now) ||
      (GetControlButtonsView() && (GetControlButtonsView()->Contains(before) ||
                                   GetControlButtonsView()->Contains(now)))) {
    UpdateControlButtonsVisibility();
  }
}

views::SlideOutController::SlideMode NotificationView::CalculateSlideMode()
  const {
  if (disable_slide_)
    return views::SlideOutController::SlideMode::kNone;

  return views::SlideOutController::SlideMode::kFull;
}

float NotificationView::GetSlideAmount() const {
  return slide_out_controller_.gesture_amount();
}

void NotificationView::DisableSlideForcibly(bool disable) {
  disable_slide_ = disable;
  slide_out_controller_.set_slide_mode(CalculateSlideMode());
}

void NotificationView::SetSlideButtonWidth(int control_button_width) {
  slide_out_controller_.SetSwipeControlWidth(control_button_width);
}

void NotificationView::SetCornerRadius(int top_radius, int bottom_radius) {
  top_radius_ = top_radius;
  bottom_radius_ = bottom_radius;
}

void NotificationView::OnCloseButtonPressed() {
  for (auto& observer : observers_) {
    observer.OnCloseButtonPressed(notification_id_);
  }
  MessagePopupView::ClosePopup(true);
}

void NotificationView::UpdateControlButtonsVisibility() {
  auto* control_buttons_view = GetControlButtonsView();
  if (control_buttons_view)
    control_buttons_view->ShowButtons(true);
}

void NotificationView::SetDrawBackgroundAsActive(bool active) {
  background()->SetNativeControlColor(active ? kHoveredButtonBackgroundColor
                                             : kNotificationBackgroundColor);
  SchedulePaint();
}

}  // namespace brave_ads

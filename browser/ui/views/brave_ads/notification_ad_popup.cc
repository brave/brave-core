/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"

#include <string>

#include "base/check_is_test.h"
#include "base/time/time.h"
#include "brave/browser/ui/brave_ads/notification_ad_delegate.h"
#include "brave/browser/ui/views/brave_ads/bounds_util.h"
#include "brave/browser/ui/views/brave_ads/color_util.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup_collection.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup_widget.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_view.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_view_factory.h"
#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "ui/accessibility/ax_enums.mojom-forward.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/display/screen.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace brave_ads {

namespace {

bool g_disable_fade_in_animation_for_testing = false;

constexpr SkColor kLightModeBorderColor = SkColorSetRGB(0xd5, 0xdb, 0xe2);
constexpr SkColor kDarkModeBorderColor = SkColorSetRGB(0x3f, 0x41, 0x45);
constexpr int kBorderThickness = 1;

#if BUILDFLAG(IS_WIN)
constexpr int kShadowElevation = 4;
constexpr int kCornerRadius = 0;
#elif BUILDFLAG(IS_MAC)
constexpr int kShadowElevation = 5;
constexpr int kCornerRadius = 7;
#elif BUILDFLAG(IS_LINUX)
constexpr int kShadowElevation = 0;
constexpr int kCornerRadius = 0;
#endif  // BUILDFLAG(IS_WIN)

SkColor GetLightModeBackgroundColor() {
  return SkColorSetRGB(0xed, 0xf0, 0xf2);
}

SkColor GetDarkModeBackgroundColor() {
  const std::string color_param =
      kCustomNotificationAdDarkModeBackgroundColor.Get();

  SkColor bg_color;
  if (!RgbStringToSkColor(color_param, &bg_color)) {
    return SkColorSetRGB(0x20, 0x23, 0x27);
  }

  return bg_color;
}

}  // namespace

NotificationAdPopup::NotificationAdPopup(
    Profile& profile,
    const NotificationAd& notification_ad,
    gfx::NativeWindow browser_native_window,
    gfx::NativeView browser_native_view)
    : profile_(profile),
      notification_ad_(notification_ad),
      animation_(std::make_unique<gfx::LinearAnimation>(this)) {
  CreatePopup(browser_native_window, browser_native_view);

  NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);

  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen_observation_.Observe(screen);
  }

  FadeIn();
}

NotificationAdPopup::~NotificationAdPopup() = default;

// static
void NotificationAdPopup::SetDisableFadeInAnimationForTesting(
    const bool disable) {
  g_disable_fade_in_animation_for_testing = disable;
}

gfx::Rect NotificationAdPopup::AdjustBoundsAndSnapToFitWorkAreaForWidget(
    views::Widget* widget,
    const gfx::Rect& bounds) {
  CHECK(widget);

  gfx::Rect fit_bounds = bounds;
  AdjustBoundsAndSnapToFitWorkAreaForNativeView(*widget, &fit_bounds);

  base::AutoReset<bool> bounds_adjust_scope(&inside_adjust_bounds_, true);
  widget->SetBounds(fit_bounds);
  return fit_bounds;
}

void NotificationAdPopup::OnDisplayAdded(const display::Display& new_display) {
  // Called when `new_display` has been added
  RecomputeAlignment();
}

void NotificationAdPopup::OnDisplaysRemoved(const display::Displays& displays) {
  // Called when `old_display` has been removed
  RecomputeAlignment();
}

void NotificationAdPopup::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t changed_metrics) {
  // Called when the metrics of a display change
  RecomputeAlignment();
}

void NotificationAdPopup::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kAlertDialog;
  node_data->SetName(
      l10n_util::GetStringUTF8(IDS_BRAVE_ADS_NOTIFICATION_AD_ACCESSIBLE_NAME));
}

void NotificationAdPopup::OnDisplayChanged() {
  // Called when the display changes (color depth or resolution)
  RecomputeAlignment();
}

void NotificationAdPopup::OnWorkAreaChanged() {
  // Called when the work area (the desktop area minus task bars, menu bars,
  // etc.) changes in size
  RecomputeAlignment();
}

void NotificationAdPopup::OnPaintBackground(gfx::Canvas* canvas) {
  CHECK(canvas);
  CHECK(GetWidget());

  gfx::Rect bounds(GetWidget()->GetLayer()->bounds());
  bounds.Inset(GetWidgetMargin());

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  // Draw border with drop shadow
  cc::PaintFlags border_flags;
  border_flags.setAntiAlias(true);
  border_flags.setColor(should_use_dark_colors ? kDarkModeBorderColor
                                               : kLightModeBorderColor);
  const gfx::ShadowDetails& shadow_details = GetShadowDetails();
  border_flags.setLooper(gfx::CreateShadowDrawLooper(shadow_details.values));
  canvas->DrawRoundRect(bounds, kCornerRadius, border_flags);

  bounds.Inset(gfx::Insets(kBorderThickness));

  // Draw background
  cc::PaintFlags background_flags;
  background_flags.setAntiAlias(true);
  background_flags.setColor(should_use_dark_colors
                                ? GetDarkModeBackgroundColor()
                                : GetLightModeBackgroundColor());
  canvas->DrawRoundRect(bounds, kCornerRadius, background_flags);
}

void NotificationAdPopup::OnThemeChanged() {
  views::View::OnThemeChanged();

  SchedulePaint();
}

bool NotificationAdPopup::OnMousePressed(const ui::MouseEvent& event) {
  initial_mouse_pressed_location_ = event.location();

  return true;
}

bool NotificationAdPopup::OnMouseDragged(const ui::MouseEvent& event) {
  const gfx::Vector2d movement =
      event.location() - initial_mouse_pressed_location_;

  if (!is_dragging_ && ExceededDragThreshold(movement)) {
    is_dragging_ = true;
  }

  if (!is_dragging_) {
    return false;
  }

  MovePopup(movement);

  return true;
}

void NotificationAdPopup::OnMouseReleased(const ui::MouseEvent& event) {
  WidgetDelegateView::OnMouseReleased(event);

  if (is_dragging_) {
    profile_->GetPrefs()->SetDouble(
        prefs::kNotificationAdLastNormalizedCoordinateX,
        last_normalized_coordinate_.x());
    profile_->GetPrefs()->SetDouble(
        prefs::kNotificationAdLastNormalizedCoordinateY,
        last_normalized_coordinate_.y());
    is_dragging_ = false;
    return;
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return;
  }

  NotificationAdDelegate* delegate = notification_ad_.delegate();
  if (delegate) {
    // This call will eventually lead to NotificationAdPopupHandler::Close call.
    delegate->OnClick();
  }
}

void NotificationAdPopup::OnWidgetDestroyed(views::Widget* widget) {
  CHECK(widget);

  CHECK(widget_observation_.IsObservingSource(widget));
  widget_observation_.Reset();

  // Remove current popup from global visible notification ad popups collection.
  NotificationAdPopupCollection::Remove(notification_ad_.id());
}

void NotificationAdPopup::OnWidgetBoundsChanged(views::Widget* widget,
                                                const gfx::Rect& new_bounds) {
  if (!inside_adjust_bounds_) {
    RecomputeAlignment();
  }
}

void NotificationAdPopup::AnimationEnded(const gfx::Animation* animation) {
  UpdateAnimation();

  switch (animation_state_) {
    case AnimationState::kIdle: {
      break;
    }

    case AnimationState::kFadeIn: {
      animation_state_ = AnimationState::kIdle;
      break;
    }

    case AnimationState::kFadeOut: {
      animation_state_ = AnimationState::kIdle;
      CloseWidgetView();
      break;
    }
  }
}

void NotificationAdPopup::AnimationProgressed(const gfx::Animation* animation) {
  UpdateAnimation();
}

void NotificationAdPopup::AnimationCanceled(const gfx::Animation* animation) {
  UpdateAnimation();
}

NotificationAd NotificationAdPopup::GetNotificationAd() const {
  return notification_ad_;
}

void NotificationAdPopup::MovePopup(const gfx::Vector2d& distance) {
  if (!IsWidgetValid()) {
    return;
  }

  const gfx::Point new_origin =
      GetWidget()->GetWindowBoundsInScreen().origin() + distance;
  const gfx::Rect adjusted_bounds = AdjustBoundsAndSnapToFitWorkAreaForWidget(
      GetWidget(), gfx::Rect(new_origin, CalculateViewSize()));
  SaveWidgetOrigin(adjusted_bounds.origin(), GetWidget()->GetNativeView());
}

void NotificationAdPopup::ClosePopup() {
  FadeOut();
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdPopup::CreatePopup(gfx::NativeWindow browser_native_window,
                                      gfx::NativeView browser_native_view) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  // Container
  views::View* container_view = new views::View();
  AddChildView(container_view);

  // Ad notification
  CHECK(!notification_ad_view_);
  notification_ad_view_ = container_view->AddChildView(
      NotificationAdViewFactory::Create(notification_ad_));

  CreateWidgetView(browser_native_window, browser_native_view);

  const gfx::Point point(GetWidgetMargin().top(), GetWidgetMargin().left());
  container_view->SetPosition(point);
  container_view->SetSize(notification_ad_view_->size());
}

bool NotificationAdPopup::DidChangePopupPosition() const {
  return profile_->GetPrefs()->HasPrefPath(
             prefs::kNotificationAdLastNormalizedCoordinateX) &&
         profile_->GetPrefs()->HasPrefPath(
             prefs::kNotificationAdLastNormalizedCoordinateY);
}

gfx::Rect NotificationAdPopup::GetInitialWidgetBounds(
    gfx::NativeView browser_native_view) {
  if (DidChangePopupPosition()) {
    last_normalized_coordinate_.set_x(profile_->GetPrefs()->GetDouble(
        prefs::kNotificationAdLastNormalizedCoordinateX));
    last_normalized_coordinate_.set_y(profile_->GetPrefs()->GetDouble(
        prefs::kNotificationAdLastNormalizedCoordinateY));
  }
  const gfx::Size size = CalculateViewSize();
  return GetWidgetBoundsForSize(size, browser_native_view);
}

gfx::Rect NotificationAdPopup::GetWidgetBoundsForSize(
    const gfx::Size& size,
    gfx::NativeView browser_native_view) {
  const gfx::Rect display_work_area =
      GetDefaultDisplayScreenWorkArea(browser_native_view);

  double normalized_display_coordinate_x =
      kCustomNotificationAdNormalizedCoordinateX.Get();
  double normalized_display_coordinate_y =
      kCustomNotificationAdNormalizedCoordinateY.Get();

  if (DidChangePopupPosition()) {
    normalized_display_coordinate_x = last_normalized_coordinate_.x();
    normalized_display_coordinate_y = last_normalized_coordinate_.y();
  }

  // Calculate position
  const double width =
      static_cast<double>(display_work_area.width() - size.width());
  const int x = static_cast<int>(width * normalized_display_coordinate_x);

  const double height =
      static_cast<double>(display_work_area.height() - size.height());
  const int y = static_cast<int>(height * normalized_display_coordinate_y);

  gfx::Point origin = display_work_area.origin();
  origin.Offset(x, y);

  // Adjust to fit display work area
  gfx::Rect bounds(origin, size);
  bounds.AdjustToFit(display_work_area);

  return bounds;
}

void NotificationAdPopup::SaveWidgetOrigin(const gfx::Point& origin,
                                           gfx::NativeView native_view) {
  const gfx::Rect display_work_area =
      GetDefaultDisplayScreenWorkArea(native_view);

  gfx::Vector2d offset = origin - display_work_area.origin();

  const gfx::Size size = CalculateViewSize();
  const double width =
      static_cast<double>(display_work_area.width() - size.width());
  last_normalized_coordinate_.set_x(offset.x() / width);

  const double height =
      static_cast<double>(display_work_area.height() - size.height());
  last_normalized_coordinate_.set_y(offset.y() / height);
}

gfx::Size NotificationAdPopup::CalculateViewSize() const {
  CHECK(notification_ad_view_);
  gfx::Size size = notification_ad_view_->size();
  CHECK(!size.IsEmpty());

  size += GetWidgetMargin().size();
  return size;
}

void NotificationAdPopup::RecomputeAlignment() {
  if (!IsWidgetValid()) {
    return;
  }

  gfx::NativeView native_view = GetWidget()->GetNativeView();
  if (GetWidget()->parent() && kUseSameZOrderAsBrowserWindow.Get()) {
    native_view = GetWidget()->parent()->GetNativeView();
  }

  const gfx::Size size = CalculateViewSize();
  const gfx::Rect widget_bounds = GetWidgetBoundsForSize(size, native_view);
  AdjustBoundsAndSnapToFitWorkAreaForWidget(GetWidget(), widget_bounds);
}

const gfx::ShadowDetails& NotificationAdPopup::GetShadowDetails() const {
  return gfx::ShadowDetails::Get(kShadowElevation, kCornerRadius);
}

gfx::Insets NotificationAdPopup::GetShadowMargin() const {
  const gfx::ShadowDetails& shadow_details = GetShadowDetails();
  return gfx::ShadowValue::GetMargin(shadow_details.values);
}

gfx::Insets NotificationAdPopup::GetWidgetMargin() const {
  gfx::Insets widget_margin(kCustomNotificationAdMargin.Get());
  widget_margin.SetToMax(-GetShadowMargin());
  return widget_margin;
}

void NotificationAdPopup::CreateWidgetView(
    gfx::NativeWindow browser_native_window,
    gfx::NativeView browser_native_view) {
  // The widget instance is owned by its NativeWidget. For more details see
  // ui/views/widget/widget.h
  NotificationAdPopupWidget* widget = new NotificationAdPopupWidget();
  widget->set_focus_on_creation(false);
  widget_observation_.Observe(widget);

  const gfx::Rect widget_bounds = GetInitialWidgetBounds(browser_native_view);
  widget->InitWidget(this, widget_bounds, browser_native_window,
                     browser_native_view);

  if (g_disable_fade_in_animation_for_testing) {
    CHECK_IS_TEST();
  } else {
    widget->SetOpacity(0.0);
  }
  const gfx::Rect bounds = widget->GetWindowBoundsInScreen();
  AdjustBoundsAndSnapToFitWorkAreaForWidget(widget, bounds);
  widget->ShowInactive();
}

void NotificationAdPopup::CloseWidgetView() {
  if (!GetWidget()) {
    DeleteDelegate();
    return;
  }

  if (GetWidget()->IsClosed()) {
    return;
  }

  GetWidget()->CloseNow();
}

void NotificationAdPopup::FadeIn() {
  if (g_disable_fade_in_animation_for_testing) {
    CHECK_IS_TEST();
    return;
  }

  animation_state_ = AnimationState::kFadeIn;

  const base::TimeDelta fade_duration =
      base::Milliseconds(kCustomNotificationAdFadeDuration.Get());
  animation_->SetDuration(fade_duration);

  StartAnimation();
}

void NotificationAdPopup::FadeOut() {
  animation_state_ = AnimationState::kFadeOut;

  const base::TimeDelta fade_duration =
      base::Milliseconds(kCustomNotificationAdFadeDuration.Get());
  animation_->SetDuration(fade_duration);

  StartAnimation();
}

void NotificationAdPopup::StartAnimation() {
  animation_->Start();

  UpdateAnimation();

  CHECK(animation_->is_animating());
}

void NotificationAdPopup::UpdateAnimation() {
  CHECK_NE(animation_state_, AnimationState::kIdle);

  if (!IsWidgetValid()) {
    return;
  }

  const double value = gfx::Tween::CalculateValue(
      animation_state_ == AnimationState::kFadeOut ? gfx::Tween::EASE_IN
                                                   : gfx::Tween::EASE_OUT,
      animation_->GetCurrentValue());

  if (animation_state_ == AnimationState::kFadeIn) {
    GetWidget()->SetOpacity(gfx::Tween::FloatValueBetween(value, 0.0f, 1.0f));
  } else if (animation_state_ == AnimationState::kFadeOut) {
    GetWidget()->SetOpacity(gfx::Tween::FloatValueBetween(value, 1.0f, 0.0f));
  }
}

bool NotificationAdPopup::IsWidgetValid() const {
  return GetWidget() && !GetWidget()->IsClosed();
}

BEGIN_METADATA(NotificationAdPopup)
END_METADATA

}  // namespace brave_ads

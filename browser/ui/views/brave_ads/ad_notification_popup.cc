/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"

#include <utility>

#include "base/time/time.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup_collection.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup_widget.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_view.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_view_factory.h"
#include "brave/browser/ui/views/brave_ads/bounds_util.h"
#include "brave/browser/ui/views/brave_ads/color_util.h"
#include "brave/components/brave_ads/common/features.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "ui/accessibility/ax_enums.mojom.h"
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
constexpr int kShadowElevation = 5;
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
      features::AdNotificationDarkModeBackgroundColor();

  SkColor bg_color;
  if (!RgbStringToSkColor(color_param, &bg_color)) {
    NOTREACHED();
    return SkColorSetRGB(0x20, 0x23, 0x27);
  }

  return bg_color;
}

void AdjustBoundsAndSnapToFitWorkAreaForWidget(views::Widget* widget,
                                               const gfx::Rect& bounds) {
  DCHECK(widget);

  gfx::Rect fit_bounds = bounds;
  const gfx::NativeView native_view = widget->GetNativeView();
  const bool should_support_multiple_displays =
      features::ShouldSupportMultipleDisplays();

  AdjustBoundsAndSnapToFitWorkAreaForNativeView(
      native_view, &fit_bounds, should_support_multiple_displays);

  widget->SetBounds(fit_bounds);
}

}  // namespace

AdNotificationPopup::AdNotificationPopup(
    Profile* profile,
    const AdNotification& ad_notification,
    gfx::NativeWindow browser_native_window,
    gfx::NativeView browser_native_view)
    : profile_(profile),
      ad_notification_(ad_notification),
      animation_(std::make_unique<gfx::LinearAnimation>(this)) {
  DCHECK(profile_);

  CreatePopup(browser_native_window, browser_native_view);

  NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);

  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->AddObserver(this);
  }

  FadeIn();
}

AdNotificationPopup::~AdNotificationPopup() {
  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->RemoveObserver(this);
  }
}

// static
void AdNotificationPopup::SetDisableFadeInAnimationForTesting(bool disable) {
  g_disable_fade_in_animation_for_testing = disable;
}

void AdNotificationPopup::OnDisplayRemoved(
    const display::Display& old_display) {
  // Called when |old_display| has been removed
  RecomputeAlignment();
}

void AdNotificationPopup::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t changed_metrics) {
  // Called when the metrics of a display change
  RecomputeAlignment();
}

void AdNotificationPopup::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kAlertDialog;
  node_data->SetName(
      l10n_util::GetStringUTF8(IDS_BRAVE_ADS_AD_NOTIFICATION_ACCESSIBLE_NAME));
}

void AdNotificationPopup::OnDisplayChanged() {
  // Called with the display changes (color depth or resolution)
  RecomputeAlignment();
}

void AdNotificationPopup::OnWorkAreaChanged() {
  // Called when the work area (the desktop area minus task bars, menu bars,
  // etc.) changes in size
  RecomputeAlignment();
}

void AdNotificationPopup::OnPaintBackground(gfx::Canvas* canvas) {
  DCHECK(canvas);

  gfx::Rect bounds(GetWidget()->GetLayer()->bounds());
  bounds.Inset(-GetShadowMargin());

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

void AdNotificationPopup::OnThemeChanged() {
  views::View::OnThemeChanged();

  SchedulePaint();
}

void AdNotificationPopup::OnWidgetDestroyed(views::Widget* widget) {
  DCHECK(widget);

  DCHECK(widget_observation_.IsObservingSource(widget));
  widget_observation_.Reset();

  // Remove current popup from global visible ad notification popups collection.
  AdNotificationPopupCollection::Remove(ad_notification_.id());
}

void AdNotificationPopup::OnWidgetBoundsChanged(views::Widget* widget,
                                                const gfx::Rect& new_bounds) {
  DCHECK(widget);

  const gfx::Point origin = new_bounds.origin();
  SaveOrigin(origin);
}

void AdNotificationPopup::AnimationEnded(const gfx::Animation* animation) {
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

void AdNotificationPopup::AnimationProgressed(const gfx::Animation* animation) {
  UpdateAnimation();
}

void AdNotificationPopup::AnimationCanceled(const gfx::Animation* animation) {
  UpdateAnimation();
}

AdNotification AdNotificationPopup::GetAdNotification() const {
  return ad_notification_;
}

void AdNotificationPopup::MovePopup(const gfx::Vector2d& distance) {
  const gfx::Rect bounds = CalculateBounds() + distance;
  AdjustBoundsAndSnapToFitWorkAreaForWidget(GetWidget(), bounds);
}

void AdNotificationPopup::ClosePopup() {
  FadeOut();
}

///////////////////////////////////////////////////////////////////////////////

void AdNotificationPopup::CreatePopup(gfx::NativeWindow browser_native_window,
                                      gfx::NativeView browser_native_view) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  // Container
  views::View* container_view = new views::View();
  AddChildView(container_view);

  // Ad notification
  DCHECK(!ad_notification_view_);
  ad_notification_view_ = container_view->AddChildView(
      AdNotificationViewFactory::Create(ad_notification_));

  const gfx::Point point(-GetShadowMargin().top(), -GetShadowMargin().left());
  container_view->SetPosition(point);
  container_view->SetSize(ad_notification_view_->size());

  CreateWidgetView(browser_native_window, browser_native_view);
}

gfx::Point AdNotificationPopup::GetDefaultOriginForSize(const gfx::Size& size) {
  const gfx::Rect display_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();

  const gfx::Rect display_work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();

  // Calculate position
  const double width = static_cast<double>(display_bounds.width());
  const double normalized_display_coordinate_x =
      features::AdNotificationNormalizedDisplayCoordinateX();
  int x = static_cast<int>(width * normalized_display_coordinate_x);
  x -= size.width() / 2.0;

  const double height = static_cast<double>(display_bounds.height());
  const double normalized_display_coordinate_y =
      features::AdNotificationNormalizedDisplayCoordinateY();
  int y = static_cast<int>(height * normalized_display_coordinate_y);
  y -= size.height() / 2.0;

  const gfx::Point origin(x, y);

  // Adjust to fit display work area
  gfx::Rect bounds(origin, size);
  bounds.AdjustToFit(display_work_area);

  // Apply insets
  const gfx::Vector2d insets(features::AdNotificationInsetX(),
                             features::AdNotificationInsetY());
  bounds += insets;

  // Adjust to fit display work area
  bounds.AdjustToFit(display_work_area);

  return bounds.origin();
}

gfx::Point AdNotificationPopup::GetOriginForSize(const gfx::Size& size) {
  if (!profile_->GetPrefs()->HasPrefPath(
          prefs::kAdNotificationLastScreenPositionX) ||
      !profile_->GetPrefs()->HasPrefPath(
          prefs::kAdNotificationLastScreenPositionY)) {
    return GetDefaultOriginForSize(size);
  }

  const int x = profile_->GetPrefs()->GetInteger(
      prefs::kAdNotificationLastScreenPositionX);
  const int y = profile_->GetPrefs()->GetInteger(
      prefs::kAdNotificationLastScreenPositionY);
  return gfx::Point(x, y);
}

void AdNotificationPopup::SaveOrigin(const gfx::Point& origin) const {
  profile_->GetPrefs()->SetInteger(prefs::kAdNotificationLastScreenPositionX,
                                   origin.x());
  profile_->GetPrefs()->SetInteger(prefs::kAdNotificationLastScreenPositionY,
                                   origin.y());
}

gfx::Size AdNotificationPopup::CalculateViewSize() const {
  DCHECK(ad_notification_view_);
  gfx::Size size = ad_notification_view_->size();
  DCHECK(!size.IsEmpty());

  size += gfx::Size(-GetShadowMargin().width(), -GetShadowMargin().height());
  return size;
}

gfx::Rect AdNotificationPopup::CalculateBounds() {
  const gfx::Size size = CalculateViewSize();
  const gfx::Point origin = GetOriginForSize(size);
  return gfx::Rect(origin, size);
}

void AdNotificationPopup::RecomputeAlignment() {
  if (!IsWidgetValid()) {
    return;
  }

  const gfx::Point window_origin =
      GetWidget()->GetWindowBoundsInScreen().origin();
  const gfx::Size view_size = CalculateViewSize();
  AdjustBoundsAndSnapToFitWorkAreaForWidget(
      GetWidget(), gfx::Rect(window_origin, view_size));
}

const gfx::ShadowDetails& AdNotificationPopup::GetShadowDetails() const {
  return gfx::ShadowDetails::Get(kShadowElevation, kCornerRadius);
}

gfx::Insets AdNotificationPopup::GetShadowMargin() const {
  const gfx::ShadowDetails& shadow_details = GetShadowDetails();
  return gfx::ShadowValue::GetMargin(shadow_details.values);
}

void AdNotificationPopup::CreateWidgetView(
    gfx::NativeWindow browser_native_window,
    gfx::NativeView browser_native_view) {
  // The widget instance is owned by its NativeWidget. For more details see
  // ui/views/widget/widget.h
  AdNotificationPopupWidget* widget = new AdNotificationPopupWidget();
  widget->set_focus_on_creation(false);
  widget_observation_.Observe(widget);

  widget->InitWidget(this, CalculateBounds(), browser_native_window,
                     browser_native_view);

  if (!g_disable_fade_in_animation_for_testing) {
    widget->SetOpacity(0.0);
  }
  const gfx::Rect bounds = widget->GetWindowBoundsInScreen();
  AdjustBoundsAndSnapToFitWorkAreaForWidget(widget, bounds);
  widget->ShowInactive();
}

void AdNotificationPopup::CloseWidgetView() {
  if (!GetWidget()) {
    DeleteDelegate();
    return;
  }

  if (GetWidget()->IsClosed()) {
    return;
  }

  GetWidget()->CloseNow();
}

void AdNotificationPopup::FadeIn() {
  if (g_disable_fade_in_animation_for_testing) {
    return;
  }

  animation_state_ = AnimationState::kFadeIn;

  const base::TimeDelta fade_duration =
      base::Milliseconds(features::AdNotificationFadeDuration());
  animation_->SetDuration(fade_duration);

  StartAnimation();
}

void AdNotificationPopup::FadeOut() {
  animation_state_ = AnimationState::kFadeOut;

  const base::TimeDelta fade_duration =
      base::Milliseconds(features::AdNotificationFadeDuration());
  animation_->SetDuration(fade_duration);

  StartAnimation();
}

void AdNotificationPopup::StartAnimation() {
  animation_->Start();

  UpdateAnimation();

  DCHECK(animation_->is_animating());
}

void AdNotificationPopup::UpdateAnimation() {
  DCHECK_NE(animation_state_, AnimationState::kIdle);

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

bool AdNotificationPopup::IsWidgetValid() const {
  return GetWidget() && !GetWidget()->IsClosed();
}

BEGIN_METADATA(AdNotificationPopup, views::WidgetDelegateView)
END_METADATA

}  // namespace brave_ads

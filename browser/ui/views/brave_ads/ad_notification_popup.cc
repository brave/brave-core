/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"

#include <map>
#include <utility>

#include "base/time/time.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_popup_widget.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_view.h"
#include "brave/browser/ui/views/brave_ads/ad_notification_view_factory.h"
#include "brave/browser/ui/views/brave_ads/bounds_util.h"
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

// TODO(https://github.com/brave/brave-browser/issues/14957): Decouple
// AdNotificationPopup management to NotificationPopupCollection
std::map<std::string, AdNotificationPopup* /* NOT OWNED */>
    g_ad_notification_popups;

bool g_disable_fade_in_animation_for_testing = false;

constexpr SkColor kLightModeBackgroundColor = SkColorSetRGB(0xed, 0xf0, 0xf2);
constexpr SkColor kDarkModeBackgroundColor = SkColorSetRGB(0x20, 0x23, 0x27);

constexpr SkColor kLightModeBorderColor = SkColorSetRGB(0xd5, 0xdb, 0xe2);
constexpr SkColor kDarkModeBorderColor = SkColorSetRGB(0x3f, 0x41, 0x45);
constexpr int kBorderThickness = 1;

#if defined(OS_WIN)
constexpr int kShadowElevation = 5;
constexpr int kCornerRadius = 0;
#elif defined(OS_MAC)
constexpr int kShadowElevation = 5;
constexpr int kCornerRadius = 7;
#elif defined(OS_LINUX)
constexpr int kShadowElevation = 0;
constexpr int kCornerRadius = 0;
#endif  // defined(OS_WIN)

class DefaultPopupInstanceFactory
    : public AdNotificationPopup::PopupInstanceFactory {
 public:
  ~DefaultPopupInstanceFactory() override = default;

  AdNotificationPopup* CreateInstance(
      Profile* profile,
      const AdNotification& ad_notification) override {
    return new AdNotificationPopup(profile, ad_notification);
  }
};

}  // namespace

AdNotificationPopup::PopupInstanceFactory::~PopupInstanceFactory() = default;

AdNotificationPopup::AdNotificationPopup(Profile* profile,
                                         const AdNotification& ad_notification)
    : profile_(profile),
      ad_notification_(ad_notification),
      animation_(std::make_unique<gfx::LinearAnimation>(this)) {
  DCHECK(profile_);

  CreatePopup();

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
void AdNotificationPopup::Show(Profile* profile,
                               const AdNotification& ad_notification) {
  DefaultPopupInstanceFactory default_factory;
  Show(profile, ad_notification, &default_factory);
}

// static
void AdNotificationPopup::Show(Profile* profile,
                               const AdNotification& ad_notification,
                               PopupInstanceFactory* popup_factory) {
  DCHECK(profile);
  DCHECK(popup_factory);

  const std::string& id = ad_notification.id();

  DCHECK(!g_ad_notification_popups[id]);
  g_ad_notification_popups[id] =
      popup_factory->CreateInstance(profile, ad_notification);

  AdNotificationDelegate* delegate = ad_notification.delegate();
  if (delegate) {
    delegate->OnShow();
  }
}

// static
void AdNotificationPopup::Close(const std::string& notification_id,
                                const bool by_user) {
  DCHECK(!notification_id.empty());

  if (!g_ad_notification_popups[notification_id]) {
    return;
  }

  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  const AdNotification ad_notification = popup->GetAdNotification();
  AdNotificationDelegate* delegate = ad_notification.delegate();
  if (delegate) {
    delegate->OnClose(by_user);
  }

  popup->FadeOut();
}

// static
void AdNotificationPopup::CloseWidget(const std::string& notification_id) {
  DCHECK(!notification_id.empty());
  if (!g_ad_notification_popups[notification_id]) {
    return;
  }

  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  popup->CloseWidgetView();
}

// static
void AdNotificationPopup::OnClick(const std::string& notification_id) {
  DCHECK(!notification_id.empty());

  DCHECK(g_ad_notification_popups[notification_id]);
  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  const AdNotification ad_notification = popup->GetAdNotification();
  AdNotificationDelegate* delegate = ad_notification.delegate();
  if (delegate) {
    delegate->OnClick();
  }

  popup->FadeOut();
}

// static
gfx::Rect AdNotificationPopup::GetBounds(const std::string& notification_id) {
  DCHECK(!notification_id.empty());

  DCHECK(g_ad_notification_popups[notification_id]);
  AdNotificationPopup* popup = g_ad_notification_popups[notification_id];
  DCHECK(popup);

  return popup->CalculateBounds();
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
  background_flags.setColor(should_use_dark_colors ? kDarkModeBackgroundColor
                                                   : kLightModeBackgroundColor);
  canvas->DrawRoundRect(bounds, kCornerRadius, background_flags);
}

void AdNotificationPopup::OnThemeChanged() {
  views::View::OnThemeChanged();

  SchedulePaint();
}

void AdNotificationPopup::OnWidgetCreated(views::Widget* widget) {
  DCHECK(widget);

  gfx::Rect bounds = widget->GetWindowBoundsInScreen();
  const gfx::NativeView native_view = widget->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);

  widget->SetBounds(bounds);
}

void AdNotificationPopup::OnWidgetDestroyed(views::Widget* widget) {
  DCHECK(widget);

  const std::string notification_id = ad_notification_.id();
  DCHECK(!notification_id.empty());

  // Note: The pointed-to AdNotificationPopup members are deallocated by their
  // containing Widgets
  g_ad_notification_popups.erase(notification_id);

  DCHECK(widget_observation_.IsObservingSource(widget));
  widget_observation_.Reset();
}

void AdNotificationPopup::OnWidgetBoundsChanged(views::Widget* widget,
                                                const gfx::Rect& new_bounds) {
  DCHECK(widget);

  const gfx::Point origin = new_bounds.origin();
  SaveOrigin(origin);
}

void AdNotificationPopup::AnimationEnded(const gfx::Animation* animation) {
  UpdateAnimation();

  const std::string notification_id = ad_notification_.id();
  DCHECK(!notification_id.empty());

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
      CloseWidget(notification_id);
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

///////////////////////////////////////////////////////////////////////////////

void AdNotificationPopup::CreatePopup() {
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

  CreateWidgetView();
}

AdNotification AdNotificationPopup::GetAdNotification() const {
  return ad_notification_;
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

gfx::Rect AdNotificationPopup::CalculateBounds() {
  DCHECK(ad_notification_view_);
  gfx::Size size = ad_notification_view_->size();
  DCHECK(!size.IsEmpty());

  size += gfx::Size(-GetShadowMargin().width(), -GetShadowMargin().height());
  const gfx::Point origin = GetOriginForSize(size);
  return gfx::Rect(origin, size);
}

void AdNotificationPopup::RecomputeAlignment() {
  if (!IsWidgetValid()) {
    return;
  }

  gfx::Rect bounds = GetWidget()->GetWindowBoundsInScreen();
  const gfx::NativeView native_view = GetWidget()->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);
  GetWidget()->SetBounds(bounds);
}

const gfx::ShadowDetails& AdNotificationPopup::GetShadowDetails() const {
  return gfx::ShadowDetails::Get(kShadowElevation, kCornerRadius);
}

gfx::Insets AdNotificationPopup::GetShadowMargin() const {
  const gfx::ShadowDetails& shadow_details = GetShadowDetails();
  return gfx::ShadowValue::GetMargin(shadow_details.values);
}

void AdNotificationPopup::CreateWidgetView() {
  // The widget instance is owned by its NativeWidget. For more details see
  // ui/views/widget/widget.h
  AdNotificationPopupWidget* widget = new AdNotificationPopupWidget();
  widget->set_focus_on_creation(false);
  widget_observation_.Observe(widget);

  widget->InitWidget(this, CalculateBounds());

  if (!g_disable_fade_in_animation_for_testing) {
    widget->SetOpacity(0.0);
  }
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
      base::TimeDelta::FromMilliseconds(features::AdNotificationFadeDuration());
  animation_->SetDuration(fade_duration);

  StartAnimation();
}

void AdNotificationPopup::FadeOut() {
  animation_state_ = AnimationState::kFadeOut;

  const base::TimeDelta fade_duration =
      base::TimeDelta::FromMilliseconds(features::AdNotificationFadeDuration());
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

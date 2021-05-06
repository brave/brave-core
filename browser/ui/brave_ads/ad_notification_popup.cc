/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_popup.h"

#include <map>
#include <utility>

#include "base/time/time.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_ads/ad_notification_view.h"
#include "brave/browser/ui/brave_ads/ad_notification_view_factory.h"
#include "brave/browser/ui/brave_ads/bounds_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/screen.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace brave_ads {

namespace {

// TODO(https://github.com/brave/brave-browser/issues/14957): Decouple
// AdNotificationPopup management to NotificationPopupCollection
std::map<std::string, AdNotificationPopup*> g_ad_notification_popups;

constexpr base::TimeDelta kFadeDuration =
    base::TimeDelta::FromMilliseconds(200);

const int kShadowElevation = 5;

constexpr SkColor kLightModeBackgroundColor = SkColorSetRGB(0xed, 0xf0, 0xf2);
constexpr SkColor kDarkModeBackgroundColor = SkColorSetRGB(0x20, 0x23, 0x27);

constexpr SkColor kLightModeBorderColor = SkColorSetRGB(0xd5, 0xdb, 0xe2);
constexpr SkColor kDarkModeBorderColor = SkColorSetRGB(0x3f, 0x41, 0x45);
const int kBorderThickness = 1;

#if defined(OS_WIN)
const int kCornerRadius = 0;
#elif defined(OS_MAC)
const int kCornerRadius = 7;
#elif defined(OS_LINUX)
const int kCornerRadius = 7;
#endif

}  // namespace

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
  DCHECK(profile);

  const std::string& id = ad_notification.id();

  DCHECK(!g_ad_notification_popups[id]);
  g_ad_notification_popups[id] =
      new AdNotificationPopup(profile, ad_notification);

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

  gfx::RectF bounds(GetWidget()->GetLayer()->bounds());
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

gfx::Point AdNotificationPopup::GetDefaultOriginForSize(
    const gfx::Size& size) const {
  const gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();

#if defined(OS_WIN)
  // Top right
  const int kTopPadding = 10;
  const int kRightPadding = 10;

  const int x = work_area.right() - (size.width() + kRightPadding);
  const int y = work_area.y() + kTopPadding;
#elif defined(OS_MAC)
  // Top right to the left of macOS native notifications
  const int kNativeNotificationWidth = 360;

  const int kTopPadding = 10;
  const int kRightPadding = 10;

  const int x = work_area.right() - kNativeNotificationWidth -
                (size.width() + kRightPadding);
  const int y = work_area.y() + kTopPadding;
#elif defined(OS_LINUX)
  // Top right
  const int kTopPadding = 10;
  const int kRightPadding = 10;

  const int x = work_area.right() - (size.width() + kRightPadding);
  const int y = work_area.y() + kTopPadding;
#endif

  return gfx::Point(x, y);
}

gfx::Point AdNotificationPopup::GetOriginForSize(const gfx::Size& size) const {
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

gfx::Rect AdNotificationPopup::CalculateBounds() const {
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
  views::Widget::InitParams params;
  params.delegate = this;
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.z_order = ui::ZOrderLevel::kFloatingWindow;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.shadow_type = views::Widget::InitParams::ShadowType::kNone;
  params.bounds = CalculateBounds();

  views::Widget* widget = new views::Widget();
  widget->set_focus_on_creation(false);
  widget_observation_.Observe(widget);

#if defined(OS_WIN)
  // We want to ensure that this toast always goes to the native desktop,
  // not the Ash desktop (since there is already another toast contents view
  // there
  if (!params.parent) {
    DCHECK(!params.native_widget);
    params.native_widget = new views::DesktopNativeWidgetAura(widget);
  }
#endif

  widget->Init(std::move(params));

  widget->SetOpacity(0.0);
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
  animation_state_ = AnimationState::kFadeIn;
  animation_->SetDuration(kFadeDuration);
  StartAnimation();
}

void AdNotificationPopup::FadeOut() {
  animation_state_ = AnimationState::kFadeOut;
  animation_->SetDuration(kFadeDuration);
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

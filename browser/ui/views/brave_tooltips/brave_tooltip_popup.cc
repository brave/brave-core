/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_popup.h"

#include <map>
#include <utility>

#include "base/time/time.h"
#include "brave/browser/ui/brave_tooltips/bounds_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
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
#include "ui/gfx/skia_paint_util.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/border.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace {

constexpr gfx::Size kTooltipSize(434 + 15, 104 + 15);

constexpr int kShadowElevation = 5;

constexpr int kBorderThickness = 6;

constexpr SkColor kLightModeBackgroundColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);
constexpr SkColor kDarkModeBackgroundColor = SkColorSetRGB(0x3B, 0x3E, 0x4F);

constexpr SkColor kBorderColor = SkColorSetRGB(0xF7, 0x3A, 0x1C);

#if BUILDFLAG(IS_WIN)
constexpr int kCornerRadius = 0;
#elif BUILDFLAG(IS_MAC)
constexpr int kCornerRadius = 7;
#elif BUILDFLAG(IS_LINUX)
constexpr int kCornerRadius = 7;
#endif

}  // namespace

namespace brave_tooltips {

BraveTooltipPopup::BraveTooltipPopup(std::unique_ptr<BraveTooltip> tooltip)
    : tooltip_(std::move(tooltip)),
      animation_(std::make_unique<gfx::LinearAnimation>(this)) {
  CreatePopup();

  NotifyAccessibilityEvent(ax::mojom::Event::kAlert, true);

  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->AddObserver(this);
  }

  FadeIn();
}

BraveTooltipPopup::~BraveTooltipPopup() {
  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->RemoveObserver(this);
  }
}

void BraveTooltipPopup::Show() {
  BraveTooltipDelegate* delegate = tooltip_->delegate();
  if (delegate) {
    delegate->OnTooltipShow(tooltip_->id());
  }
}

void BraveTooltipPopup::Close() {
  BraveTooltipDelegate* delegate = tooltip_->delegate();
  if (delegate) {
    delegate->OnTooltipClose(tooltip_->id());
  }

  FadeOut();
}

void BraveTooltipPopup::CloseWidget() {
  CloseWidgetView();
}

void BraveTooltipPopup::OnOkButtonPressed() {
  tooltip_->PerformOkButtonAction();

  BraveTooltipDelegate* delegate = tooltip_->delegate();
  if (delegate) {
    delegate->OnTooltipOkButtonPressed(tooltip_->id());
  }

  FadeOut();
}

void BraveTooltipPopup::OnCancelButtonPressed() {
  if (!tooltip_->attributes().cancel_button_enabled()) {
    return;
  }

  tooltip_->PerformCancelButtonAction();

  BraveTooltipDelegate* delegate = tooltip_->delegate();
  if (delegate) {
    delegate->OnTooltipCancelButtonPressed(tooltip_->id());
  }

  FadeOut();
}

void BraveTooltipPopup::set_normalized_display_coordinates(double x, double y) {
  normalized_display_coordinate_x_ = x;
  normalized_display_coordinate_y_ = y;
}

void BraveTooltipPopup::set_display_work_area_insets(int x, int y) {
  display_work_area_inset_x_ = x;
  display_work_area_inset_y_ = y;
}

void BraveTooltipPopup::OnDisplaysRemoved(
    const display::Displays& old_displays) {
  // Called when |old_display| has been removed
  RecomputeAlignment();
}

void BraveTooltipPopup::OnDisplayMetricsChanged(const display::Display& display,
                                                uint32_t changed_metrics) {
  // Called when the metrics of a display change
  RecomputeAlignment();
}

void BraveTooltipPopup::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kAlertDialog;
  node_data->SetName(l10n_util::GetStringUTF8(
      IDS_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_ACCESSIBLE_NAME));
}

void BraveTooltipPopup::OnDisplayChanged() {
  // Called with the display changes (color depth or resolution)
  RecomputeAlignment();
}

void BraveTooltipPopup::OnWorkAreaChanged() {
  // Called when the work area (the desktop area minus task bars, menu bars,
  // etc.) changes in size
  RecomputeAlignment();
}

void BraveTooltipPopup::OnPaintBackground(gfx::Canvas* canvas) {
  DCHECK(canvas);

  gfx::Rect bounds(GetWidget()->GetLayer()->bounds());
  bounds.Inset(-GetShadowMargin());

  const bool should_use_dark_colors = GetNativeTheme()->ShouldUseDarkColors();

  // Draw border
  canvas->FillRect(gfx::Rect(0, 0, kBorderThickness, bounds.bottom()),
                   kBorderColor);

  // Draw drop shadow
  cc::PaintFlags shadow_flags;
  shadow_flags.setAntiAlias(true);
  const gfx::ShadowDetails& shadow_details = GetShadowDetails();
  shadow_flags.setLooper(gfx::CreateShadowDrawLooper(shadow_details.values));
  canvas->DrawRoundRect(bounds, kCornerRadius, shadow_flags);

  // Draw background
  cc::PaintFlags background_flags;
  background_flags.setAntiAlias(true);
  background_flags.setColor(should_use_dark_colors ? kDarkModeBackgroundColor
                                                   : kLightModeBackgroundColor);
  canvas->DrawRect(bounds, background_flags);
}

void BraveTooltipPopup::OnThemeChanged() {
  views::View::OnThemeChanged();

  SchedulePaint();
}

void BraveTooltipPopup::OnWidgetCreated(views::Widget* widget) {
  DCHECK(widget);

  gfx::Rect bounds = widget->GetWindowBoundsInScreen();
  const gfx::NativeView native_view = widget->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);

  widget->SetBounds(bounds);
}

void BraveTooltipPopup::OnWidgetDestroyed(views::Widget* widget) {
  DCHECK(widget);

  DCHECK(widget_observation_.IsObservingSource(widget));
  widget_observation_.Reset();

  BraveTooltipDelegate* delegate = tooltip_->delegate();
  if (delegate) {
    delegate->OnTooltipWidgetDestroyed(tooltip_->id());
  }
}

void BraveTooltipPopup::OnWidgetBoundsChanged(views::Widget* widget,
                                              const gfx::Rect& new_bounds) {
  DCHECK(widget);
  widget_origin_ = new_bounds.origin();
}

void BraveTooltipPopup::AnimationEnded(const gfx::Animation* animation) {
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
      CloseWidget();
      break;
    }
  }
}

void BraveTooltipPopup::AnimationProgressed(const gfx::Animation* animation) {
  UpdateAnimation();
}

void BraveTooltipPopup::AnimationCanceled(const gfx::Animation* animation) {
  UpdateAnimation();
}

///////////////////////////////////////////////////////////////////////////////

void BraveTooltipPopup::CreatePopup() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets()));

  // Tooltip
  DCHECK(!tooltip_view_);
  tooltip_view_ =
      AddChildView(new BraveTooltipView(this, tooltip_->attributes()));

  CreateWidgetView();
}

gfx::Point BraveTooltipPopup::GetDefaultOriginForSize(const gfx::Size& size) {
  const gfx::Rect display_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();

  const gfx::Rect display_work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();

  // Calculate position
  const double width = static_cast<double>(display_bounds.width());
  int x = static_cast<int>(width * normalized_display_coordinate_x_);
  x -= size.width() / 2.0;

  const double height = static_cast<double>(display_bounds.height());
  int y = static_cast<int>(height * normalized_display_coordinate_y_);
  y -= size.height() / 2.0;

  const gfx::Point origin(x, y);

  // Adjust to fit display work area
  gfx::Rect bounds(origin, size);
  bounds.AdjustToFit(display_work_area);

  // Apply insets
  const gfx::Vector2d insets(display_work_area_inset_x_,
                             display_work_area_inset_y_);
  bounds += insets;

  // Adjust to fit display work area
  bounds.AdjustToFit(display_work_area);

  return bounds.origin();
}

gfx::Rect BraveTooltipPopup::CalculateBounds(bool use_default_origin) {
  DCHECK(tooltip_view_);
  gfx::Size size = tooltip_view_->size();
  size.set_height(kTooltipSize.height());
  DCHECK(!size.IsEmpty());

  const gfx::Point origin =
      use_default_origin ? GetDefaultOriginForSize(size) : widget_origin_;
  return gfx::Rect(origin, size);
}

void BraveTooltipPopup::RecomputeAlignment() {
  if (!IsWidgetValid()) {
    return;
  }

  gfx::Rect bounds = GetWidget()->GetWindowBoundsInScreen();
  const gfx::NativeView native_view = GetWidget()->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);

  GetWidget()->SetBounds(bounds);
}

const gfx::ShadowDetails& BraveTooltipPopup::GetShadowDetails() const {
  return gfx::ShadowDetails::Get(kShadowElevation, kCornerRadius);
}

gfx::Insets BraveTooltipPopup::GetShadowMargin() const {
  const gfx::ShadowDetails& shadow_details = GetShadowDetails();
  gfx::Insets shadow_margin =
      gfx::ShadowValue::GetMargin(shadow_details.values);
  shadow_margin.set_left(-kBorderThickness);
  shadow_margin.set_top(0);
  return shadow_margin;
}

void BraveTooltipPopup::CreateWidgetView() {
  // The widget instance is owned by its NativeWidget. For more details see
  // ui/views/widget/widget.h
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.delegate = this;
  params.z_order = ui::ZOrderLevel::kFloatingWindow;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.shadow_type = views::Widget::InitParams::ShadowType::kNone;
  params.bounds = CalculateBounds(true);

  views::Widget* widget = new views::Widget();
  widget->set_focus_on_creation(false);
  widget_observation_.Observe(widget);

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
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

void BraveTooltipPopup::CloseWidgetView() {
  if (!GetWidget()) {
    DeleteDelegate();
    return;
  }

  if (GetWidget()->IsClosed()) {
    return;
  }

  GetWidget()->CloseNow();
}

void BraveTooltipPopup::FadeIn() {
  animation_state_ = AnimationState::kFadeIn;
  animation_->SetDuration(base::Milliseconds(fade_duration_));
  StartAnimation();
}

void BraveTooltipPopup::FadeOut() {
  animation_state_ = AnimationState::kFadeOut;
  animation_->SetDuration(base::Milliseconds(fade_duration_));
  StartAnimation();
}

void BraveTooltipPopup::StartAnimation() {
  animation_->Start();

  UpdateAnimation();

  DCHECK(animation_->is_animating());
}

void BraveTooltipPopup::UpdateAnimation() {
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

bool BraveTooltipPopup::IsWidgetValid() const {
  return GetWidget() && !GetWidget()->IsClosed();
}

BEGIN_METADATA(BraveTooltipPopup)
END_METADATA

}  // namespace brave_tooltips

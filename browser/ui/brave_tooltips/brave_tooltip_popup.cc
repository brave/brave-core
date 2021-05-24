/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_tooltips/brave_tooltip_popup.h"

#include <map>
#include <utility>

#include "base/time/time.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_tooltips/bounds_util.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_view.h"
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

#if defined(OS_WIN)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace {

std::map<std::string, brave_tooltips::BraveTooltipPopup*> g_tooltip_popups;

constexpr SkColor kBackgroundColor = SkColorSetRGB(0xFF, 0xFF, 0xFF);

}  // namespace

namespace brave_tooltips {

BraveTooltipPopup::BraveTooltipPopup(Profile* profile,
                                     const BraveTooltip& tooltip)
    : profile_(profile),
      tooltip_(tooltip),
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

BraveTooltipPopup::~BraveTooltipPopup() {
  display::Screen* screen = display::Screen::GetScreen();
  if (screen) {
    screen->RemoveObserver(this);
  }
}

// static
void BraveTooltipPopup::Show(Profile* profile, const BraveTooltip& tooltip) {
  DCHECK(profile);

  const std::string id = tooltip.id();

  DCHECK(!g_tooltip_popups[id]);
  g_tooltip_popups[id] = new BraveTooltipPopup(profile, tooltip);

  BraveTooltipDelegate* delegate = tooltip.delegate();
  if (delegate) {
    delegate->OnShow();
  }
}

// static
void BraveTooltipPopup::Close(const std::string& tooltip_id,
                              const bool by_user) {
  DCHECK(!tooltip_id.empty());

  if (!g_tooltip_popups[tooltip_id]) {
    return;
  }

  BraveTooltipPopup* popup = g_tooltip_popups[tooltip_id];
  DCHECK(popup);

  const BraveTooltip tooltip = popup->GetTooltip();
  BraveTooltipDelegate* delegate = tooltip.delegate();
  if (delegate) {
    delegate->OnClose(by_user);
  }

  popup->FadeOut();
}

// static
void BraveTooltipPopup::CloseWidget(const std::string& tooltip_id) {
  DCHECK(!tooltip_id.empty());
  if (!g_tooltip_popups[tooltip_id]) {
    return;
  }

  BraveTooltipPopup* popup = g_tooltip_popups[tooltip_id];
  DCHECK(popup);

  popup->CloseWidgetView();
}

// static
void BraveTooltipPopup::OnOkButtonPressed(const std::string& tooltip_id) {
  DCHECK(!tooltip_id.empty());

  DCHECK(g_tooltip_popups[tooltip_id]);
  BraveTooltipPopup* popup = g_tooltip_popups[tooltip_id];
  DCHECK(popup);

  const BraveTooltip tooltip = popup->GetTooltip();
  BraveTooltipDelegate* delegate = tooltip.delegate();
  if (delegate) {
    delegate->OnOkButtonPressed();
  }

  popup->FadeOut();
}

// static
void BraveTooltipPopup::OnCancelButtonPressed(const std::string& tooltip_id) {
  DCHECK(!tooltip_id.empty());

  DCHECK(g_tooltip_popups[tooltip_id]);
  BraveTooltipPopup* popup = g_tooltip_popups[tooltip_id];
  DCHECK(popup);

  const BraveTooltip tooltip = popup->GetTooltip();
  BraveTooltipDelegate* delegate = tooltip.delegate();
  if (delegate) {
    delegate->OnCancelButtonPressed();
  }

  popup->FadeOut();
}

void BraveTooltipPopup::set_normalized_display_coordinates(double x, double y) {
  normalized_display_coordinate_x_ = x;
  normalized_display_coordinate_y_ = y;
}

void BraveTooltipPopup::set_display_work_area_insets(int x, int y) {
  display_work_area_inset_x_ = x;
  display_work_area_inset_y_ = y;
}

void BraveTooltipPopup::OnDisplayRemoved(const display::Display& old_display) {
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

  gfx::RectF bounds(GetWidget()->GetLayer()->bounds());

  // Draw background
  cc::PaintFlags background_flags;
  background_flags.setAntiAlias(true);
  background_flags.setColor(kBackgroundColor);
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

  const std::string tooltip_id = tooltip_.id();
  DCHECK(!tooltip_id.empty());
  g_tooltip_popups.erase(tooltip_id);

  DCHECK(widget_observation_.IsObservingSource(widget));
  widget_observation_.Reset();
}

void BraveTooltipPopup::AnimationEnded(const gfx::Animation* animation) {
  UpdateAnimation();

  const std::string tooltip_id = tooltip_.id();
  DCHECK(!tooltip_id.empty());

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
      CloseWidget(tooltip_id);
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

  // Container
  views::View* container_view = new views::View();
  AddChildView(container_view);

  // Tooltip
  DCHECK(!tooltip_view_);
  tooltip_view_ = container_view->AddChildView(new BraveTooltipView(tooltip_));

  container_view->SetPosition(gfx::Point(0, 0));
  container_view->SetSize(tooltip_view_->size());

  CreateWidgetView();
}

BraveTooltip BraveTooltipPopup::GetTooltip() const {
  return tooltip_;
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

gfx::Rect BraveTooltipPopup::CalculateBounds() {
  DCHECK(tooltip_view_);
  gfx::Size size = tooltip_view_->size();
  DCHECK(!size.IsEmpty());

  const gfx::Point origin = GetDefaultOriginForSize(size);
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

void BraveTooltipPopup::CreateWidgetView() {
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

  const base::TimeDelta fade_duration =
      base::TimeDelta::FromMilliseconds(fade_duration_);
  animation_->SetDuration(fade_duration);

  StartAnimation();
}

void BraveTooltipPopup::FadeOut() {
  animation_state_ = AnimationState::kFadeOut;

  const base::TimeDelta fade_duration =
      base::TimeDelta::FromMilliseconds(fade_duration_);
  animation_->SetDuration(fade_duration);

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

BEGIN_METADATA(BraveTooltipPopup, views::WidgetDelegateView)
END_METADATA

}  // namespace brave_tooltips

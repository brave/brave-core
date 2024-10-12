/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

namespace gfx {
class LinearAnimation;
class Point;
class Rect;
class Size;
}  // namespace gfx

namespace views {
class Widget;
}  // namespace views

namespace brave_tooltips {

class BraveTooltipView;

// Provides a generalized interface for displaying user-actionable tooltips on
// the desktop window.
//
// Usage example:
//   auto tooltip = std::make_unique<brave_tooltips::BraveTooltip>(
//       "id", brave_tooltips::BraveTooltipAttributes(u"Title", u"Body", u"OK"),
//       this);
//   auto popup = std::make_unique<brave_tooltips::BraveTooltipPopup>(
//       std::move(tooltip));
//   popup->Show();
//   ...
//   popup->Close();
//
// This creates and show a tooltip with the given attributes/controls. Button
// presseses will forward to the delegate (set via the BraveTooltip
// constructor). Finally, the tooltip is closed.
class BraveTooltipPopup : public views::WidgetDelegateView,
                          public views::WidgetObserver,
                          public gfx::AnimationDelegate,
                          public display::DisplayObserver {
  METADATA_HEADER(BraveTooltipPopup, views::WidgetDelegateView)
 public:
  explicit BraveTooltipPopup(std::unique_ptr<BraveTooltip> tooltip);
  ~BraveTooltipPopup() override;

  BraveTooltipPopup(const BraveTooltipPopup&) = delete;
  BraveTooltipPopup& operator=(const BraveTooltipPopup&) = delete;

  // Show the tooltip popup view
  void Show();

  // Close the tooltip popup view
  void Close();

  // Close the widget
  void CloseWidget();

  // User pressed the Ok button
  void OnOkButtonPressed();

  // User pressed the Cancel button
  void OnCancelButtonPressed();

  gfx::Rect CalculateBounds(bool use_default_origin);

  void set_normalized_display_coordinates(double x, double y);

  void set_display_work_area_insets(int x, int y);

  views::Button* ok_button_for_testing() const {
    return tooltip_view_ ? tooltip_view_->ok_button_for_testing() : nullptr;
  }

  views::Button* cancel_button_for_testing() const {
    return tooltip_view_ ? tooltip_view_->cancel_button_for_testing() : nullptr;
  }

  // display::DisplayObserver:
  void OnDisplaysRemoved(const display::Displays& old_displays) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // views::WidgetDelegateView:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnDisplayChanged() override;
  void OnWorkAreaChanged() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

  // views::WidgetObserver:
  void OnWidgetCreated(views::Widget* widget) override;
  void OnWidgetDestroyed(views::Widget* widget) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  // AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

 private:
  void CreatePopup();

  gfx::Point GetDefaultOriginForSize(const gfx::Size& size);

  void RecomputeAlignment();

  const gfx::ShadowDetails& GetShadowDetails() const;
  gfx::Insets GetShadowMargin() const;

  void CreateWidgetView();
  void CloseWidgetView();

  bool IsWidgetValid() const;

  void StartAnimation();
  void UpdateAnimation();

  void FadeIn();
  void FadeOut();

  std::unique_ptr<BraveTooltip> tooltip_;

  raw_ptr<BraveTooltipView> tooltip_view_ = nullptr;

  gfx::Point widget_origin_ = {0, 0};

  double normalized_display_coordinate_x_ = 1.0;
  double normalized_display_coordinate_y_ = 0.0;

  int display_work_area_inset_x_ = -13;
  int display_work_area_inset_y_ = 18;

  int fade_duration_ = 200;

  enum class AnimationState {
    kIdle,
    kFadeIn,
    kFadeOut,
  };

  const std::unique_ptr<gfx::LinearAnimation> animation_;
  AnimationState animation_state_ = AnimationState::kIdle;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_H_

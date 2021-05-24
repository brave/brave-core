/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

class Profile;

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

// The widget delegate of a tooltip popup. The view is owned by
// the widget
class BraveTooltipPopup : public views::WidgetDelegateView,
                          public views::WidgetObserver,
                          public gfx::AnimationDelegate,
                          public display::DisplayObserver {
 public:
  METADATA_HEADER(BraveTooltipPopup);

  explicit BraveTooltipPopup(Profile* profile, const BraveTooltip& tooltip);
  ~BraveTooltipPopup() override;

  // Show the tooltip popup view for the given |profile| and
  // |tooltip|
  static void Show(Profile* profile, const BraveTooltip& tooltip);

  // Close the tooltip popup view for the given |tooltip_id|.
  // |by_user| is true if the tooltip popup was closed by the user,
  // otherwise false
  static void Close(const std::string& tooltip_id, const bool by_user);

  // Close the widget for the given |tooltip_id|
  static void CloseWidget(const std::string& tooltip_id);

  // User pressed the Ok button for the given |tooltip_id|
  static void OnOkButtonPressed(const std::string& tooltip_id);

  // User pressed the Cancel button for the given |tooltip_id|
  static void OnCancelButtonPressed(const std::string& tooltip_id);

  void set_normalized_display_coordinates(double x, double y);

  void set_display_work_area_insets(int x, int y);

  // display::DisplayObserver:
  void OnDisplayRemoved(const display::Display& old_display) override;
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

  // AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

 private:
  enum class AnimationState {
    // No animation is running
    kIdle,

    // Fading in a tooltip
    kFadeIn,

    // Fading out a tooltip
    kFadeOut,
  };

  Profile* profile_;  // NOT OWNED

  void CreatePopup();

  BraveTooltip tooltip_;
  BraveTooltip GetTooltip() const;

  gfx::Point GetDefaultOriginForSize(const gfx::Size& size);

  gfx::Rect CalculateBounds();

  void RecomputeAlignment();

  void CreateWidgetView();
  void CloseWidgetView();

  BraveTooltipView* tooltip_view_ = nullptr;  // NOT OWNED

  double normalized_display_coordinate_x_ = 1.0;
  double normalized_display_coordinate_y_ = 0.0;

  int display_work_area_inset_x_ = -13;
  int display_work_area_inset_y_ = 18;

  int fade_duration_ = 200;

  void FadeIn();
  void FadeOut();

  const std::unique_ptr<gfx::LinearAnimation> animation_;
  AnimationState animation_state_ = AnimationState::kIdle;
  void StartAnimation();
  void UpdateAnimation();

  bool IsWidgetValid() const;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  BraveTooltipPopup(const BraveTooltipPopup&) = delete;
  BraveTooltipPopup& operator=(const BraveTooltipPopup&) = delete;
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_POPUP_H_

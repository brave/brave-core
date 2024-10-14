/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_H_

#include <cstdint>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

class Profile;

namespace display {
class Screen;
}  // namespace display

namespace gfx {
class LinearAnimation;
class Insets;
class Point;
class Rect;
class Size;
struct ShadowDetails;
class Vector2d;
}  // namespace gfx

namespace views {
class Widget;
}  // namespace views

namespace brave_ads {

class NotificationAdView;

// The widget delegate of an notification ad popup. The view is owned by the
// widget
class NotificationAdPopup : public views::WidgetDelegateView,
                            public views::WidgetObserver,
                            public gfx::AnimationDelegate,
                            public display::DisplayObserver {
  METADATA_HEADER(NotificationAdPopup, views::WidgetDelegateView)
 public:

  NotificationAdPopup(Profile* profile,
                      const NotificationAd& notification_ad,
                      gfx::NativeWindow browser_native_window,
                      gfx::NativeView browser_native_view);

  NotificationAdPopup(const NotificationAdPopup&) = delete;
  NotificationAdPopup& operator=(const NotificationAdPopup&) = delete;

  NotificationAdPopup(NotificationAdPopup&&) noexcept = delete;
  NotificationAdPopup& operator=(NotificationAdPopup&&) noexcept = delete;

  ~NotificationAdPopup() override;

  // Disables fade in animation for snapshot tests.
  static void SetDisableFadeInAnimationForTesting(bool disable);

  gfx::Rect AdjustBoundsAndSnapToFitWorkAreaForWidget(views::Widget* widget,
                                                      const gfx::Rect& bounds);

  // display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplaysRemoved(const display::Displays& displays) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

  // views::WidgetDelegateView:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnDisplayChanged() override;
  void OnWorkAreaChanged() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;

  // views::WidgetObserver:
  void OnWidgetDestroyed(views::Widget* widget) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  // AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

  NotificationAd GetNotificationAd() const;
  void MovePopup(const gfx::Vector2d& distance);
  void ClosePopup();

 private:
  enum class AnimationState {
    // No animation is running
    kIdle,

    // Fading in a notification ad
    kFadeIn,

    // Fading out a notification ad
    kFadeOut
  };

  void CreatePopup(gfx::NativeWindow browser_native_window,
                   gfx::NativeView browser_native_view);

  bool DidChangePopupPosition() const;
  gfx::Rect GetInitialWidgetBounds(gfx::NativeView browser_native_view);
  gfx::Rect GetWidgetBoundsForSize(const gfx::Size& size,
                                   gfx::NativeView browser_native_view);
  void SaveWidgetOrigin(const gfx::Point& origin, gfx::NativeView native_view);

  gfx::Size CalculateViewSize() const;

  void RecomputeAlignment();

  const gfx::ShadowDetails& GetShadowDetails() const;
  gfx::Insets GetShadowMargin() const;
  gfx::Insets GetWidgetMargin() const;

  void CreateWidgetView(gfx::NativeWindow browser_native_window,
                        gfx::NativeView browser_native_view);
  void CloseWidgetView();

  void FadeIn();
  void FadeOut();

  void StartAnimation();
  void UpdateAnimation();

  bool IsWidgetValid() const;

  raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED

  NotificationAd notification_ad_;

  raw_ptr<NotificationAdView> notification_ad_view_ = nullptr;

  const std::unique_ptr<gfx::LinearAnimation> animation_;
  AnimationState animation_state_ = AnimationState::kIdle;

  gfx::Point initial_mouse_pressed_location_;
  bool is_dragging_ = false;

  bool inside_adjust_bounds_ = false;

  gfx::PointF last_normalized_coordinate_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  base::ScopedObservation<display::Screen, display::DisplayObserver>
      screen_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_H_

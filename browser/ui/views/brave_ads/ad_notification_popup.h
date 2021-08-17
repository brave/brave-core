/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

class Profile;

namespace gfx {
class LinearAnimation;
class Insets;
class Point;
class Rect;
class Size;
struct ShadowDetails;
}  // namespace gfx

namespace views {
class Widget;
}  // namespace views

namespace brave_ads {

class AdNotificationView;

// The widget delegate of an ad notification popup. The view is owned by the
// widget
class AdNotificationPopup : public views::WidgetDelegateView,
                            public views::WidgetObserver,
                            public gfx::AnimationDelegate,
                            public display::DisplayObserver {
 public:
  // Creates instance of AdNotificationPopup. Can be used in tests to specify
  // AdNotificationPopup instance which is created on AdNotificationPopup::Show
  // call.
  class PopupInstanceFactory {
   public:
    virtual ~PopupInstanceFactory();

    virtual AdNotificationPopup* CreateInstance(
        Profile* profile,
        const AdNotification& ad_notification) = 0;
  };

  METADATA_HEADER(AdNotificationPopup);

  explicit AdNotificationPopup(Profile* profile,
                               const AdNotification& ad_notification);
  ~AdNotificationPopup() override;

  // Show the notification popup view for the given |profile| and
  // |ad_notification|
  static void Show(Profile* profile, const AdNotification& ad_notification);

  // Show the notification popup view for the given |profile| and
  // |ad_notification|. Popup instance is created using |popup_factory|
  static void Show(Profile* profile,
                   const AdNotification& ad_notification,
                   PopupInstanceFactory* popup_factory);

  // Close the notification popup view for the given |notification_id|.
  // |by_user| is true if the notification popup was closed by the user,
  // otherwise false
  static void Close(const std::string& notification_id, const bool by_user);

  // Close the widget for the given |notification_id|
  static void CloseWidget(const std::string& notification_id);

  // User clicked the notification popup view for the given |notification_id|
  static void OnClick(const std::string& notification_id);

  // Return the bounds for the given |notification_id|
  static gfx::Rect GetBounds(const std::string& notification_id);

  // Disables fade in animation for snapshot tests.
  static void SetDisableFadeInAnimationForTesting(bool disable);

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
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  // AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

 private:
  enum class AnimationState {
    // No animation is running
    kIdle,

    // Fading in an ad notification
    kFadeIn,

    // Fading out an ad notidfication
    kFadeOut,
  };

  Profile* profile_;  // NOT OWNED

  void CreatePopup();

  AdNotification ad_notification_;
  AdNotification GetAdNotification() const;

  gfx::Point GetDefaultOriginForSize(const gfx::Size& size);
  gfx::Point GetOriginForSize(const gfx::Size& size);
  void SaveOrigin(const gfx::Point& origin) const;

  gfx::Rect CalculateBounds();

  void RecomputeAlignment();

  const gfx::ShadowDetails& GetShadowDetails() const;
  gfx::Insets GetShadowMargin() const;

  void CreateWidgetView();
  void CloseWidgetView();

  AdNotificationView* ad_notification_view_ = nullptr;  // NOT OWNED

  void FadeIn();
  void FadeOut();

  const std::unique_ptr<gfx::LinearAnimation> animation_;
  AnimationState animation_state_ = AnimationState::kIdle;
  void StartAnimation();
  void UpdateAnimation();

  bool IsWidgetValid() const;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  AdNotificationPopup(const AdNotificationPopup&) = delete;
  AdNotificationPopup& operator=(const AdNotificationPopup&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_H_

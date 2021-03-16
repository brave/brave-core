/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_ADS_NOTIFICATION_VIEW_H_
#define BRAVE_UI_BRAVE_ADS_NOTIFICATION_VIEW_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/ui/brave_ads/public/cpp/notification.h"
#include "brave/ui/brave_ads/public/cpp/notification_delegate.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/animation/slide_out_controller.h"
#include "ui/views/animation/slide_out_controller_delegate.h"
#include "ui/views/controls/focus_ring.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace views {
class ScrollView;
}  // namespace views

namespace brave_ads {

class Notification;
class NotificationControlButtonsView;

class NotificationView : public views::InkDropHostView,
                         public views::SlideOutControllerDelegate,
                         public views::FocusChangeListener {
 public:
  explicit NotificationView(const Notification& notification);
  class Observer {
   public:
    virtual ~Observer() = default;

    virtual void OnSlideStarted(const std::string& notification_id) {}
    virtual void OnSlideChanged(const std::string& notification_id) {}
    virtual void OnPreSlideOut(const std::string& notification_id) {}
    virtual void OnSlideOut(const std::string& notification_id) {}
    virtual void OnCloseButtonPressed(const std::string& notification_id) {}
  };

  ~NotificationView() override;
  virtual void UpdateWithNotification(const Notification& notification);
  virtual NotificationControlButtonsView* GetControlButtonsView() const = 0;
  virtual void CloseSwipeControl();
  virtual void SlideOutAndClose(int direction);

  virtual void UpdateCornerRadius(int top_radius, int bottom_radius);

  // Invoked when the container view of NotificationView
  // is starting the animation that possibly hides some part of
  // the NotificationView.
  // During the animation, NotificationView should comply with the
  // Z order in views.
  virtual void OnContainerAnimationStarted();
  virtual void OnContainerAnimationEnded();
  void OnCloseButtonPressed();

  void OnPaint(gfx::Canvas* canvas) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void RemovedFromWidget() override;
  void AddedToWidget() override;
  void OnThemeChanged() override;

  // views::SlideOutControllerDelegate:
  ui::Layer* GetSlideOutLayer() override;
  void OnSlideStarted() override;
  void OnSlideChanged(bool in_progress) override;
  void OnSlideOut() override;

  // views::FocusChangeListener:
  void OnWillChangeFocus(views::View* before, views::View* now) override;
  void OnDidChangeFocus(views::View* before, views::View* now) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Gets the current horizontal scroll offset of the view by slide gesture.
  float GetSlideAmount() const;

  // Disables slide by vertical swipe regardless of the current notification
  // mode.
  void DisableSlideForcibly(bool disable);

  // Updates the width of the buttons which are hidden and avail by swipe.
  void SetSlideButtonWidth(int coutrol_button_width);

  void set_scroller(views::ScrollView* scroller) { scroller_ = scroller; }
  std::string notification_id() const { return notification_id_; }

 protected:
  virtual void UpdateControlButtonsVisibility();

  // Changes the background color and schedules a paint.
  virtual void SetDrawBackgroundAsActive(bool active);

  void SetCornerRadius(int top_radius, int bottom_radius);

  views::ScrollView* scroller() { return scroller_; }

  base::ObserverList<Observer>::Unchecked* observers() { return &observers_; }

 private:
  class HighlightPathGenerator;

  // Gets the highlight path for the notification based on bounds and corner
  // radii.
  SkPath GetHighlightPath() const;

  // Returns the ideal slide mode by calculating the current status.
  views::SlideOutController::SlideMode CalculateSlideMode() const;

  std::string notification_id_;
  views::ScrollView* scroller_ = nullptr;

  views::SlideOutController slide_out_controller_;
  base::ObserverList<Observer>::Unchecked observers_;

  // True if the slide is disabled forcibly.
  bool disable_slide_ = false;

  views::FocusManager* focus_manager_ = nullptr;
  std::unique_ptr<views::FocusRing> focus_ring_;

  // Radius values used to determine the rounding for the rounded rectangular
  // shape of the notification.
  int top_radius_ = 0;
  int bottom_radius_ = 0;

  DISALLOW_COPY_AND_ASSIGN(NotificationView);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_NOTIFICATION_VIEW_H_

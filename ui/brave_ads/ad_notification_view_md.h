/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_ADS_AD_NOTIFICATION_VIEW_MD_H_
#define BRAVE_UI_BRAVE_ADS_AD_NOTIFICATION_VIEW_MD_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "brave/ui/brave_ads/notification_view.h"
#include "ui/views/animation/ink_drop_observer.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/textfield/textfield_controller.h"

namespace views {
class Label;
}

namespace brave_ads {

class NotificationHeaderView;

// View that displays all current types of notification (web, basic, image, and
// list) except the custom notification. Future notification types may be
// handled by other classes, in which case instances of those classes would be
// returned by the Create() factory method below.
class AdNotificationViewMD : public NotificationView,
                             public views::InkDropObserver {
 public:
  explicit AdNotificationViewMD(const Notification& notification);
  ~AdNotificationViewMD() override;

  void Activate();

  void AddBackgroundAnimation(const ui::Event& event);
  void RemoveBackgroundAnimation();

  void AddLayerBeneathView(ui::Layer* layer) override;
  void RemoveLayerBeneathView(ui::Layer* layer) override;
  void Layout() override;
  void OnFocus() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  void PreferredSizeChanged() override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDropMask> CreateInkDropMask() const override;
  SkColor GetInkDropBaseColor() const override;
  void UpdateWithNotification(const Notification& notification) override;
  void UpdateCornerRadius(int top_radius, int bottom_radius) override;
  NotificationControlButtonsView* GetControlButtonsView() const override;

  void InkDropAnimationStarted() override;
  void InkDropRippleAnimationEnded(views::InkDropState ink_drop_state) override;

 private:
  class AdNotificationViewMDPathGenerator;

  void UpdateControlButtonsVisibilityWithNotification(
      const Notification& notification);

  void CreateOrUpdateViews(const Notification& notification);

  void CreateOrUpdateContextTitleView(const Notification& notification);
  void CreateOrUpdateNotificationView(const Notification& notification);
  void CreateOrUpdateActionButtonViews(const Notification& notification);

  void ToggleInlineSettings(const ui::Event& event);

  // Returns the list of children which need to have their layers created or
  // destroyed when the ink drop is visible.
  std::vector<views::View*> GetChildrenForLayerAdjustment() const;

  views::InkDropContainerView* const ink_drop_container_;

  // View containing close and settings buttons
  std::unique_ptr<NotificationControlButtonsView> control_buttons_view_;

  // Describes whether the view should display a hand pointer or not.
  bool clickable_;

  // Container views directly attached to this view.
  NotificationHeaderView* header_row_ = nullptr;
  views::View* content_row_ = nullptr;
  views::View* actions_row_ = nullptr;
  views::View* settings_row_ = nullptr;

  // Containers for left and right side on |content_row_|
  views::View* left_content_ = nullptr;
  views::View* right_content_ = nullptr;

  // Views which are dynamically created inside view hierarchy.
  views::Label* message_view_ = nullptr;
  views::View* action_buttons_row_ = nullptr;

  // Counter for view layouting, which is used during the CreateOrUpdate*
  // phases to keep track of the view ordering. See crbug.com/901045
  int left_content_count_;

  // Owned by views properties. Guaranteed to be not null for the lifetime of
  // |this| because views properties are the last thing cleaned up.
  AdNotificationViewMDPathGenerator* highlight_path_generator_ = nullptr;

  std::unique_ptr<ui::EventHandler> click_activator_;

  base::TimeTicks last_mouse_pressed_timestamp_;

  base::WeakPtrFactory<AdNotificationViewMD> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(AdNotificationViewMD);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_AD_NOTIFICATION_VIEW_MD_H_

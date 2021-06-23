/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_VIEW_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_VIEW_H_

#include "brave/browser/ui/brave_ads/ad_notification.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/metadata/metadata_header_macros.h"

namespace gfx {
class Canvas;
class Point;
}  // namespace gfx

namespace brave_ads {

class AdNotificationView : public views::InkDropHostView {
 public:
  METADATA_HEADER(AdNotificationView);

  explicit AdNotificationView(const AdNotification& ad_notification);
  ~AdNotificationView() override;

  // Update notification contents to |ad_notification|
  virtual void UpdateContents(const AdNotification& ad_notification);

  void OnCloseButtonPressed();

  // views::InkDropHostView:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override;
  void OnThemeChanged() override;

 private:
  AdNotification ad_notification_;

  gfx::Point initial_mouse_pressed_location_;
  bool is_dragging_ = false;

  bool is_closing_ = false;

  void CreateView();

  std::u16string accessible_name_;
  void MaybeNotifyAccessibilityEvent();

  AdNotificationView(const AdNotificationView&) = delete;
  AdNotificationView& operator=(const AdNotificationView&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_VIEW_H_

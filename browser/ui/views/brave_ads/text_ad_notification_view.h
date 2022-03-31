/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_TEXT_AD_NOTIFICATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_TEXT_AD_NOTIFICATION_VIEW_H_

#include "brave/browser/ui/views/brave_ads/ad_notification_view.h"

namespace views {
class Label;
class View;
}  // namespace views

namespace brave_ads {

class AdNotification;
class AdNotificationControlButtonsView;
class AdNotificationHeaderView;

class TextAdNotificationView : public AdNotificationView {
 public:
  explicit TextAdNotificationView(const AdNotification& ad_notification);
  ~TextAdNotificationView() override;

  // AdNotificationView:
  void UpdateContents(const AdNotification& ad_notification) override;
  void OnThemeChanged() override;

 private:
  AdNotification ad_notification_;

  void CreateView(const AdNotification& ad_notification);

  AdNotificationHeaderView* CreateHeaderView(
      const AdNotification& ad_notification);

  views::Label* body_label_ = nullptr;  // NOT OWNED

  views::View* CreateBodyView(const AdNotification& ad_notification);
  views::Label* CreateBodyLabel(const AdNotification& ad_notification);
  void UpdateBodyLabel();

  TextAdNotificationView(const TextAdNotificationView&) = delete;
  TextAdNotificationView& operator=(const TextAdNotificationView&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_TEXT_AD_NOTIFICATION_VIEW_H_

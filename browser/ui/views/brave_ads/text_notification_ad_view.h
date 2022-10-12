/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_TEXT_NOTIFICATION_AD_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_TEXT_NOTIFICATION_AD_VIEW_H_

#include "brave/browser/ui/views/brave_ads/notification_ad_view.h"

namespace views {
class Label;
class View;
}  // namespace views

namespace brave_ads {

class NotificationAd;
class NotificationAdControlButtonsView;
class NotificationAdHeaderView;

class TextNotificationAdView : public NotificationAdView {
 public:
  explicit TextNotificationAdView(const NotificationAd& notification_ad);

  TextNotificationAdView(const TextNotificationAdView&) = delete;
  TextNotificationAdView& operator=(const TextNotificationAdView&) = delete;

  TextNotificationAdView(TextNotificationAdView&& other) noexcept = delete;
  TextNotificationAdView& operator=(TextNotificationAdView&& other) noexcept =
      delete;

  ~TextNotificationAdView() override;

  // NotificationAdView:
  void UpdateContents(const NotificationAd& notification_ad) override;
  void OnThemeChanged() override;

 private:
  NotificationAd notification_ad_;

  void CreateView(const NotificationAd& notification_ad);

  NotificationAdHeaderView* CreateHeaderView(
      const NotificationAd& notification_ad);

  views::Label* body_label_ = nullptr;  // NOT OWNED

  views::View* CreateBodyView(const NotificationAd& notification_ad);
  views::Label* CreateBodyLabel(const NotificationAd& notification_ad);
  void UpdateBodyLabel();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_TEXT_NOTIFICATION_AD_VIEW_H_

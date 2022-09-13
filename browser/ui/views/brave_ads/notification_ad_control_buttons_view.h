/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_CONTROL_BUTTONS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_CONTROL_BUTTONS_VIEW_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace views {
class Button;
}  // namespace views

namespace brave_ads {

class NotificationAdView;
class PaddedImageButton;
class PaddedImageView;

class NotificationAdControlButtonsView : public views::View {
 public:
  METADATA_HEADER(NotificationAdControlButtonsView);

  explicit NotificationAdControlButtonsView(
      NotificationAdView* notification_ad_view);
  ~NotificationAdControlButtonsView() override;

  NotificationAdControlButtonsView(const NotificationAdControlButtonsView&) =
      delete;
  NotificationAdControlButtonsView& operator=(
      const NotificationAdControlButtonsView&) = delete;

  // views::View:
  void OnThemeChanged() override;

  void UpdateContent();

 private:
  NotificationAdView* notification_ad_view_ = nullptr;  // NOT OWNED

  PaddedImageView* info_button_ = nullptr;
  PaddedImageButton* close_button_ = nullptr;

  void CreateView();

  void CreateInfoButton();
  void UpdateInfoButton();

  void CreateCloseButton();
  void UpdateCloseButton();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_CONTROL_BUTTONS_VIEW_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_CONTROL_BUTTONS_VIEW_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_CONTROL_BUTTONS_VIEW_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace views {
class Button;
}

namespace brave_ads {

class AdNotificationView;
class PaddedImageButton;
class PaddedImageView;

class AdNotificationControlButtonsView : public views::View {
 public:
  METADATA_HEADER(AdNotificationControlButtonsView);

  explicit AdNotificationControlButtonsView(
      AdNotificationView* ad_notification_view);
  ~AdNotificationControlButtonsView() override;

  void UpdateContent();

 private:
  AdNotificationView* ad_notification_view_ = nullptr;  // NOT OWNED

  PaddedImageView* info_button_ = nullptr;
  PaddedImageButton* close_button_ = nullptr;

  void CreateView();

  void CreateInfoButton();
  void UpdateInfoButton();

  void CreateCloseButton();
  void UpdateCloseButton();

  AdNotificationControlButtonsView(const AdNotificationControlButtonsView&) =
      delete;
  AdNotificationControlButtonsView& operator=(
      const AdNotificationControlButtonsView&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_CONTROL_BUTTONS_VIEW_H_

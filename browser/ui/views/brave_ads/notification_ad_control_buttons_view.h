/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_CONTROL_BUTTONS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_CONTROL_BUTTONS_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
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
  METADATA_HEADER(NotificationAdControlButtonsView, views::View)
 public:

  explicit NotificationAdControlButtonsView(
      NotificationAdView& notification_ad_view);

  NotificationAdControlButtonsView(const NotificationAdControlButtonsView&) =
      delete;
  NotificationAdControlButtonsView& operator=(
      const NotificationAdControlButtonsView&) = delete;

  ~NotificationAdControlButtonsView() override;

  // views::View:
  void OnThemeChanged() override;

  void UpdateContent();

 private:
  void CreateView();

  void CreateInfoButton();
  void UpdateInfoButton();

  void CreateCloseButton();
  void UpdateCloseButton();

  const raw_ref<NotificationAdView> notification_ad_view_;

  raw_ptr<PaddedImageView> info_button_ = nullptr;
  raw_ptr<PaddedImageButton> close_button_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_CONTROL_BUTTONS_VIEW_H_

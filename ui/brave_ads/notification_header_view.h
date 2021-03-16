// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_ADS_NOTIFICATION_HEADER_VIEW_H_
#define BRAVE_UI_BRAVE_ADS_NOTIFICATION_HEADER_VIEW_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/timer/timer.h"
#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/button/button.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace brave_ads {

class NotificationHeaderView : public views::Button {
 public:
  explicit NotificationHeaderView(PressedCallback callback = PressedCallback());
  ~NotificationHeaderView() override;
  void SetAdIcon(const gfx::ImageSkia& img);
  void SetAdName(const std::u16string& name);
  void SetAdNameElideBehavior(gfx::ElideBehavior elide_behavior);
  void SetOverflowIndicator(int count);

  // Set the unified theme color used among the app icon, app name, and expand
  // button.
  void SetAccentColor(SkColor color);

  // Sets the background color of the notification. This is used to ensure that
  // the accent color has enough contrast against the background.
  void SetBackgroundColor(SkColor color);

  void ClearAdIcon();
  void SetSubpixelRenderingEnabled(bool enabled);

  // Shows or hides the app icon.
  void SetAdIconVisible(bool visible);

  SkColor accent_color_for_testing() { return accent_color_; }

  const views::Label* summary_text_for_testing() const {
    return summary_text_view_;
  }

  const std::u16string& ad_name_for_testing() const;

  const gfx::ImageSkia& ad_icon_for_testing() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(NotificationHeaderViewTest, SettingsMode);

  // Update visibility for both |summary_text_view_| and |timestamp_view_|.
  void UpdateSummaryTextVisibility();

  SkColor accent_color_ = kNotificationDefaultAccentColor;

  views::ImageView* ad_icon_view_ = nullptr;
  views::Label* ad_name_view_ = nullptr;
  views::View* detail_views_ = nullptr;
  views::Label* summary_text_divider_ = nullptr;
  views::Label* summary_text_view_ = nullptr;

  bool using_default_ad_icon_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationHeaderView);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_NOTIFICATION_HEADER_VIEW_H_

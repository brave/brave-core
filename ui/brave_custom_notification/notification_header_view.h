// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_NOTIFICATION_HEADER_VIEW_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_NOTIFICATION_HEADER_VIEW_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/timer/timer.h"
#include "ui/gfx/text_constants.h"
#include "brave/ui/brave_custom_notification/public/cpp/constants.h"
#include "ui/views/controls/button/button.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

namespace brave_custom_notification {

class NotificationHeaderView : public views::Button {
 public:
  explicit NotificationHeaderView(views::ButtonListener* listener);
  ~NotificationHeaderView() override;
  void SetAppIcon(const gfx::ImageSkia& img);
  void SetAppName(const base::string16& name);
  void SetAppNameElideBehavior(gfx::ElideBehavior elide_behavior);

  // Only show AppIcon and AppName in settings mode.
  // void SetDetailViewsVisible(bool visible);

  // Progress, summary and overflow indicator are all the same UI element so are
  // mutually exclusive.
  void SetProgress(int progress);
  void SetSummaryText(const base::string16& text);
  void SetOverflowIndicator(int count);

  // void SetExpandButtonEnabled(bool enabled);
  // void SetExpanded(bool expanded);

  // Set the unified theme color used among the app icon, app name, and expand
  // button.
  void SetAccentColor(SkColor color);

  // Sets the background color of the notification. This is used to ensure that
  // the accent color has enough contrast against the background.
  void SetBackgroundColor(SkColor color);

  void ClearAppIcon();
  void SetSubpixelRenderingEnabled(bool enabled);

  // Shows or hides the app icon.
  void SetAppIconVisible(bool visible);

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  views::ImageView* expand_button() { return expand_button_; }

  SkColor accent_color_for_testing() { return accent_color_; }

  const views::Label* summary_text_for_testing() const {
    return summary_text_view_;
  }

  const base::string16& app_name_for_testing() const;

  const gfx::ImageSkia& app_icon_for_testing() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(NotificationHeaderViewTest, SettingsMode);

  // Update visibility for both |summary_text_view_| and |timestamp_view_|.
  void UpdateSummaryTextVisibility();

  SkColor accent_color_ = kNotificationDefaultAccentColor;

  views::ImageView* app_icon_view_ = nullptr;
  views::Label* app_name_view_ = nullptr;
  views::View* detail_views_ = nullptr;
  views::Label* summary_text_divider_ = nullptr;
  views::Label* summary_text_view_ = nullptr;
  views::ImageView* expand_button_ = nullptr;

  bool has_progress_ = false;
  // bool is_expanded_ = false;
  bool using_default_app_icon_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationHeaderView);
};

}

#endif

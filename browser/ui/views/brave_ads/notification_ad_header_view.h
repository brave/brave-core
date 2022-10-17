/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_HEADER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_HEADER_VIEW_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/view.h"

namespace views {
class Label;
}  // namespace views

namespace brave_ads {

class NotificationAdHeaderView : public views::View {
 public:
  METADATA_HEADER(NotificationAdHeaderView);

  explicit NotificationAdHeaderView(const int width);

  NotificationAdHeaderView(const NotificationAdHeaderView&) = delete;
  NotificationAdHeaderView& operator=(const NotificationAdHeaderView&) = delete;

  NotificationAdHeaderView(NotificationAdHeaderView&& other) noexcept = delete;
  NotificationAdHeaderView& operator=(
      NotificationAdHeaderView&& other) noexcept = delete;

  ~NotificationAdHeaderView() override;

  void SetTitle(const std::u16string& name);
  void SetTitleElideBehavior(gfx::ElideBehavior elide_behavior);

  void UpdateContent();

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 private:
  void CreateView(const int width);

  views::Label* title_label_ = nullptr;
  views::Label* CreateTitleLabel();
  void UpdateTitleLabel();
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_HEADER_VIEW_H_

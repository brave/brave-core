/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_HEADER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_HEADER_VIEW_H_

#include <memory>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/view.h"

namespace views {
class Label;
}  // namespace views

namespace brave_ads {

class NotificationAdHeaderView : public views::View {
  METADATA_HEADER(NotificationAdHeaderView, views::View)
 public:
  NotificationAdHeaderView();

  NotificationAdHeaderView(const NotificationAdHeaderView&) = delete;
  NotificationAdHeaderView& operator=(const NotificationAdHeaderView&) = delete;

  ~NotificationAdHeaderView() override;

  void SetTitle(const std::u16string& name);
  void SetTitleElideBehavior(gfx::ElideBehavior elide_behavior);

  void UpdateContent();

  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnThemeChanged() override;

 private:
  void CreateView();

  std::unique_ptr<views::Label> CreateTitleLabel();
  void UpdateTitleLabel();

  raw_ptr<views::Label> title_label_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_HEADER_VIEW_H_

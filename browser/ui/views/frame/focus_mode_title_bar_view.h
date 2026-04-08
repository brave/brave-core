/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TITLE_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TITLE_BAR_VIEW_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class ImageView;
class Label;
}  // namespace views

// Small title bar shown at the top of the browser window when Focus Mode is
// enabled. Displays the active tab's favicon followed by a short label
// (registered domain for http/https, otherwise scheme://host).
class FocusModeTitleBarView : public views::View {
  METADATA_HEADER(FocusModeTitleBarView, views::View)

 public:
  FocusModeTitleBarView();
  FocusModeTitleBarView(const FocusModeTitleBarView&) = delete;
  FocusModeTitleBarView& operator=(const FocusModeTitleBarView&) = delete;
  ~FocusModeTitleBarView() override;

  // Points this view at the given WebContents. |contents| may be null.
  void SetWebContents(content::WebContents* contents);

  // views::View:
  void OnThemeChanged() override;

 private:
  void Update();

  raw_ptr<views::ImageView> favicon_ = nullptr;
  raw_ptr<views::Label> domain_label_ = nullptr;
  raw_ptr<content::WebContents> web_contents_ = nullptr;
  std::optional<base::CallbackListSubscription> tab_ui_updated_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TITLE_BAR_VIEW_H_

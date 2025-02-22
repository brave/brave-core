/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace views {
class WebView;
class Widget;
}

namespace content {
class WebContents;
}

class EmailAliasesBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(EmailAliasesBubbleView, views::BubbleDialogDelegateView)

 public:
  static void Show(content::WebContents* web_contents,
                   views::View* anchor_view,
                   uint64_t field_renderer_id);
  static void Close();
  static void FillFieldWithNewAlias(const std::string& value);

  EmailAliasesBubbleView(views::View* anchor_view,
                         content::WebContents* web_contents,
                         uint64_t field_renderer_id);
  ~EmailAliasesBubbleView() override;

  void FillField(const std::string& value);

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  void OnWidgetDestroying(views::Widget* widget) override;
  void OnAnchorBoundsChanged() override;

 private:
  raw_ptr<content::WebContents> web_contents_;
  uint64_t field_renderer_id_;
  raw_ptr<views::WebView> web_view_;
  bool widget_is_being_destroyed_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_

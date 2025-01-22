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
}

class Browser;

class EmailAliasesBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(EmailAliasesBubbleView, views::BubbleDialogDelegateView)

 public:
  static void Show(Browser* browser, uint64_t field_renderer_id);
  static void Close();
  static void FillFieldWithNewAlias(const std::string& value);

  EmailAliasesBubbleView(views::View* anchor_view,
                         Browser* browser,
                         uint64_t field_renderer_id);
  ~EmailAliasesBubbleView() override;

  void FillField(const std::string& value);

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  raw_ptr<Browser> browser_;
  uint64_t field_renderer_id_;
  raw_ptr<views::WebView> web_view_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_

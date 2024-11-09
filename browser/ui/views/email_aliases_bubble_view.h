/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

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

  // views::BubbleDialogDelegateView override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;

  void FillField(const std::string& value);

 private:
  raw_ptr<Browser> browser_;
  uint64_t field_renderer_id_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_EMAIL_ALIASES_BUBBLE_VIEW_H_

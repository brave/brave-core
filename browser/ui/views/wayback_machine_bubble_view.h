/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_BUBBLE_VIEW_H_

#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class Browser;

class WaybackMachineBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(WaybackMachineBubbleView, views::BubbleDialogDelegateView)

 public:
  static void Show(Browser* browser, views::View* anchor);

  WaybackMachineBubbleView(content::WebContents::Getter wc_getter,
                           views::View* anchor);
  ~WaybackMachineBubbleView() override;

  // views::BubbleDialogDelegateView override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;

 private:
  void OnAccepted();
  content::WebContents* GetWebContents();

  content::WebContents::Getter wc_getter_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_BUBBLE_VIEW_H_

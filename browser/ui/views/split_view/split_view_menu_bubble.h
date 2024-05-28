/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_MENU_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_MENU_BUBBLE_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/metadata/view_factory.h"

class Browser;

// A bubble view that shows up when a menu button on the SplitViewSeparator is
// clicked. This contains operations that can be performed on the split view.
class SplitViewMenuBubble : public views::BubbleDialogDelegateView {
  METADATA_HEADER(SplitViewMenuBubble, views::BubbleDialogDelegateView)
 public:
  static void Show(Browser* browser, views::View* anchor);
  ~SplitViewMenuBubble() override;

  // views::BubbleDialogDelegateView:
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;

 private:
  SplitViewMenuBubble(Browser* browser, views::View* anchor);

  void OnClose();
};

BEGIN_VIEW_BUILDER(/*no export*/,
                   SplitViewMenuBubble,
                   views::BubbleDialogDelegateView)
END_VIEW_BUILDER

DEFINE_VIEW_BUILDER(/*no export*/, SplitViewMenuBubble)

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_MENU_BUBBLE_H_

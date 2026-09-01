/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_BUBBLE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace actions {
class ActionItem;
}  // namespace actions

namespace content {
class WebContents;
}  // namespace content

class WaybackMachineBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(WaybackMachineBubbleView, views::BubbleDialogDelegateView)

 public:
  static void Show(content::WebContents* web_contents,
                   views::View* anchor,
                   actions::ActionItem* item);

  WaybackMachineBubbleView(base::WeakPtr<content::WebContents> web_contents,
                           views::View* anchor,
                           actions::ActionItem* item);
  ~WaybackMachineBubbleView() override;

 private:
  void OnAccepted();

  base::WeakPtr<content::WebContents> web_contents_;
  raw_ptr<actions::ActionItem> item_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_WAYBACK_MACHINE_BUBBLE_VIEW_H_

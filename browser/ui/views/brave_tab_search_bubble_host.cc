/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_tab_search_bubble_host.h"

#include "ui/views/bubble/bubble_dialog_delegate_view.h"

void BraveTabSearchBubbleHost::SetBubbleArrow(
    views::BubbleBorder::Arrow arrow) {
  arrow_ = arrow;
}

bool BraveTabSearchBubbleHost::ShowTabSearchBubble(
    bool triggered_by_keyboard_shortcut) {
  bool result =
      TabSearchBubbleHost::ShowTabSearchBubble(triggered_by_keyboard_shortcut);

  if (!arrow_ || !result) {
    return result;
  }

  auto* widget = webui_bubble_manager_.GetBubbleWidget();
  DCHECK(widget && widget->widget_delegate());

  auto* bubble_delegate = widget->widget_delegate()->AsBubbleDialogDelegate();
  DCHECK(bubble_delegate);

  bubble_delegate->SetArrow(*arrow_);
  return result;
}

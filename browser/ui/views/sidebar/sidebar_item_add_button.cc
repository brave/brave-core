/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"

#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"

SidebarItemAddButton::SidebarItemAddButton(BraveBrowser* browser)
    : SidebarButtonView(nullptr), browser_(browser) {}

SidebarItemAddButton::~SidebarItemAddButton() = default;

void SidebarItemAddButton::OnMouseEntered(const ui::MouseEvent& event) {
  SidebarButtonView::OnMouseEntered(event);
  ShowBubble();
}

void SidebarItemAddButton::OnGestureEvent(ui::GestureEvent* event) {
  SidebarButtonView::OnGestureEvent(event);
  if (event->type() == ui::ET_GESTURE_TAP)
    ShowBubble();
}

void SidebarItemAddButton::OnWidgetDestroying(views::Widget* widget) {
  observation_.Reset();
}

void SidebarItemAddButton::ShowBubble() {
  if (observation_.IsObserving())
    return;

  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(
      new SidebarAddItemBubbleDelegateView(browser_, this));
  observation_.Observe(bubble);
  bubble->Show();
}

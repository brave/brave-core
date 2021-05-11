/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"

#include "base/time/time.h"
#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"

SidebarItemAddButton::SidebarItemAddButton(BraveBrowser* browser)
    : SidebarButtonView(nullptr), browser_(browser) {}

SidebarItemAddButton::~SidebarItemAddButton() = default;

void SidebarItemAddButton::OnMouseEntered(const ui::MouseEvent& event) {
  SidebarButtonView::OnMouseEntered(event);
  ShowBubbleWithDelay();
}

void SidebarItemAddButton::OnMouseExited(const ui::MouseEvent& event) {
  SidebarButtonView::OnMouseEntered(event);
  // Don't show bubble if user goes outo from add item quickly.
  timer_.Stop();
}

void SidebarItemAddButton::OnGestureEvent(ui::GestureEvent* event) {
  SidebarButtonView::OnGestureEvent(event);
  if (event->type() == ui::ET_GESTURE_TAP) {
    // Show bubble immediately after tapping.
    DoShowBubble();
  }
}

void SidebarItemAddButton::OnWidgetDestroying(views::Widget* widget) {
  observation_.Reset();
}

void SidebarItemAddButton::ShowBubbleWithDelay() {
  if (observation_.IsObserving())
    return;

  if (timer_.IsRunning())
    timer_.Stop();

  constexpr int kBubbleLaunchDelayInMS = 200;
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromMilliseconds(kBubbleLaunchDelayInMS), this,
               &SidebarItemAddButton::DoShowBubble);
}

void SidebarItemAddButton::DoShowBubble() {
  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(
      new SidebarAddItemBubbleDelegateView(browser_, this));
  observation_.Observe(bubble);
  bubble->Show();
}

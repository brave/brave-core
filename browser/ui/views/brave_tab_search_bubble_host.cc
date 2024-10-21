/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_tab_search_bubble_host.h"

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

void BraveTabSearchBubbleHost::SetBubbleArrow(
    views::BubbleBorder::Arrow arrow) {
  arrow_ = arrow;
}

bool BraveTabSearchBubbleHost::ShowTabSearchBubble(
    bool triggered_by_keyboard_shortcut,
    tab_search::mojom::TabSearchSection section,
    tab_search::mojom::TabOrganizationFeature organization_feature) {
  bool result = TabSearchBubbleHost::ShowTabSearchBubble(
      triggered_by_keyboard_shortcut, section, organization_feature);
  if (!arrow_ || !result) {
    return result;
  }

  auto* widget = webui_bubble_manager_->GetBubbleWidget();
  DCHECK(widget && widget->widget_delegate());

  auto* bubble_delegate = widget->widget_delegate()->AsBubbleDialogDelegate();
  DCHECK(bubble_delegate);

  auto* anchor_widget = button_->GetWidget();
  DCHECK(anchor_widget);
  anchor_widget = anchor_widget->GetTopLevelWidget();
  DCHECK(anchor_widget);

#if DCHECK_IS_ON()
  // This path is reachable only when it's vertical tabs.
  auto* browser_view = BrowserView::GetBrowserViewForNativeWindow(
      anchor_widget->GetNativeWindow());
  DCHECK(browser_view);
  DCHECK(tabs::utils::ShouldShowVerticalTabs(browser_view->browser()));
#endif

  bubble_delegate->SetArrow(*arrow_);

  if (anchor_widget->IsFullscreen()) {
    // In this case, anchor bubble onto the screen edge. we should also reparent
    // native widget, as vertical tab's widget could be hidden.
    gfx::Rect bounds = anchor_widget->GetWorkAreaBoundsInScreen();
    int offset = GetLayoutConstant(TAB_PRE_TITLE_PADDING);
    bubble_delegate->SetAnchorView(nullptr);
    bubble_delegate->set_parent_window(anchor_widget->GetNativeView());
    bubble_delegate->SetAnchorRect(
        gfx::Rect(bounds.x() + offset, bounds.y() + offset, 0, 0));

    views::Widget::ReparentNativeView(widget->GetNativeView(),
                                      anchor_widget->GetNativeView());
    bubble_delegate->SizeToContents();
  }

  widget->Show();
  return result;
}

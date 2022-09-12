/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_EDIT_ITEM_BUBBLE_DELEGATE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_EDIT_ITEM_BUBBLE_DELEGATE_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "ui/base/metadata/metadata_macros_internal.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/textfield/textfield_controller.h"

namespace views {
class Textfield;
}  // namespace views

class BraveBrowser;

class SidebarEditItemBubbleDelegateView
    : public views::BubbleDialogDelegateView,
      public views::TextfieldController {
 public:
  METADATA_HEADER(SidebarEditItemBubbleDelegateView);
  SidebarEditItemBubbleDelegateView(BraveBrowser* browser,
                                    const sidebar::SidebarItem& item,
                                    views::View* anchor_view);
  ~SidebarEditItemBubbleDelegateView() override;
  SidebarEditItemBubbleDelegateView(const SidebarEditItemBubbleDelegateView&) =
      delete;
  SidebarEditItemBubbleDelegateView& operator=(
      const SidebarEditItemBubbleDelegateView&) = delete;

  // views::BubbleDialogDelegateView overrides:
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;
  views::View* GetInitiallyFocusedView() override;
  void AddedToWidget() override;

  // views::TextfieldController overrides:
  void ContentsChanged(views::Textfield* sender,
                       const std::u16string& new_contents) override;

 private:
  void AddChildViews();
  void UpdateItem();
  void UpdateOKButtonEnabledState();

  sidebar::SidebarItem target_item_;
  raw_ptr<BraveBrowser> browser_ = nullptr;
  raw_ptr<views::Textfield> title_tf_ = nullptr;
  raw_ptr<views::Textfield> url_tf_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_EDIT_ITEM_BUBBLE_DELEGATE_VIEW_H_

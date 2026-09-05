/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_ONION_LOCATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_ONION_LOCATION_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Profile;

class OnionLocationView : public PageActionIconView {
  METADATA_HEADER(OnionLocationView, PageActionIconView)
 public:
  OnionLocationView(Profile* profile,
                    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
                    PageActionIconView::Delegate* page_action_icon_delegate);
  OnionLocationView(const OnionLocationView&) = delete;
  OnionLocationView& operator=(const OnionLocationView&) = delete;
  ~OnionLocationView() override;

 private:
  // PageActionIconView:
  const gfx::VectorIcon& GetVectorIcon() const override;
  void UpdateImpl() override;
  views::BubbleDialogDelegate* GetBubble() const override;
  void OnExecuting(PageActionIconView::ExecuteSource execute_source) override;

  raw_ptr<Profile> profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_ONION_LOCATION_VIEW_H_

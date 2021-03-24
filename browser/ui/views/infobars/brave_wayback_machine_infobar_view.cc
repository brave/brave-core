/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_view.h"

#include <string>
#include <utility>

#include "brave/browser/infobars/brave_wayback_machine_delegate_impl.h"
#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_contents_view.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"

// static
std::unique_ptr<infobars::InfoBar>
BraveWaybackMachineDelegateImpl::CreateInfoBarView(
    std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
    content::WebContents* contents) {
  return std::make_unique<BraveWaybackMachineInfoBarView>(std::move(delegate),
                                                          contents);
}

BraveWaybackMachineInfoBarView::BraveWaybackMachineInfoBarView(
      std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
      content::WebContents* contents)
    : InfoBarView(std::move(delegate)) {
  sub_views_ = new BraveWaybackMachineInfoBarContentsView(contents);
  sub_views_->SizeToPreferredSize();
  AddChildView(sub_views_);
}

BraveWaybackMachineInfoBarView::~BraveWaybackMachineInfoBarView() {
}

void BraveWaybackMachineInfoBarView::Layout() {
  InfoBarView::Layout();
  // |sub_views_| occupies from the beginning.
  // Don't adjust child view's height. Just use their preferred height.
  // It can cause infinite Layout loop because of infobar's height
  // re-calculation during the animation.
  sub_views_->SetBounds(0, OffsetY(sub_views_), GetEndX(),
                        sub_views_->height());
}

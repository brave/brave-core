/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_delegate_impl.h"

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

BraveMultiContentsViewDelegateImpl::BraveMultiContentsViewDelegateImpl(
    BrowserWindowInterface& browser)
    : MultiContentsViewDelegateImpl(browser), bwi_(browser) {}

BraveMultiContentsViewDelegateImpl::~BraveMultiContentsViewDelegateImpl() =
    default;

void BraveMultiContentsViewDelegateImpl::WebContentsFocused(
    content::WebContents* contents) {
  TabStripModel* model = bwi_->GetTabStripModel();
  // https://github.com/brave/brave-browser/issues/53121
  // On macOS, closing a split view detaches a web contents native view, which
  // can synchronously trigger a focus change (via AppKit first responder
  // transfer). This focus event propagates to ActivateTabAt(), but if we're
  // already inside CloseAllTabs(), the TabStripModel reentrancy guard fires.
  // Skip the activation when tabs are being closed.
  if (model->closing_all()) {
    return;
  }

  MultiContentsViewDelegateImpl::WebContentsFocused(contents);
}

void BraveMultiContentsViewDelegateImpl::ResizeWebContents(double ratio,
                                                           bool done_resizing) {
  // Upstream assumes active tab is split tab when resizing happens.
  // But, split tab is not active tab when panel is active.
  // TODO(https://github.com/brave/brave-browser/issues/33533):
  // Need to handle split view resize when web panel is active.
  // If not skip, crash happened now due to above reason.
  if (!bwi_->GetTabStripModel()->GetActiveTab()->GetSplit()) {
    return;
  }

  MultiContentsViewDelegateImpl::ResizeWebContents(ratio, done_resizing);
}

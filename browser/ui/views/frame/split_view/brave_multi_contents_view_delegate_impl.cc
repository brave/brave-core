/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_delegate_impl.h"

#include "base/types/to_address.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"

BraveMultiContentsViewDelegateImpl::BraveMultiContentsViewDelegateImpl(
    Browser& browser)
    : MultiContentsViewDelegateImpl(browser),
      browser_(browser),
      tab_strip_model_(*browser.tab_strip_model()) {}

BraveMultiContentsViewDelegateImpl::~BraveMultiContentsViewDelegateImpl() =
    default;

void BraveMultiContentsViewDelegateImpl::WebContentsFocused(
    content::WebContents* contents) {
  // https://github.com/brave/brave-browser/issues/53121
  // On macOS, closing a split view detaches a web contents native view, which
  // can synchronously trigger a focus change (via AppKit first responder
  // transfer). This focus event propagates to ActivateTabAt(), but if we're
  // already inside CloseAllTabs(), the TabStripModel reentrancy guard fires.
  // Skip the activation when tabs are being closed.
  if (tab_strip_model_->closing_all()) {
    return;
  }

  // Web panel's tab is a plain pinned tab, not part of a split, so
  // upstream's split-only activation logic never activates it when
  // its contents area is clicked/focused. Likewise, it never reactivates
  // the normal tab that gets clicked back into after the panel became
  // active. Activate directly in both cases.
  if (sidebar::IsWebPanelRelatedFocusChange(base::to_address(browser_),
                                            contents)) {
    if (tabs::TabInterface* tab =
            tabs::TabInterface::MaybeGetFromContents(contents);
        tab && tab_strip_model_->GetActiveTab() != tab) {
      tab_strip_model_->ActivateTabAt(tab_strip_model_->GetIndexOfTab(tab));
    }
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
  if (!tab_strip_model_->GetActiveTab()->GetSplit()) {
    return;
  }

  MultiContentsViewDelegateImpl::ResizeWebContents(ratio, done_resizing);
}

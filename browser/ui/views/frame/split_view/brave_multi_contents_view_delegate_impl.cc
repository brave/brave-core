/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_delegate_impl.h"

#include "base/types/to_address.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"

BraveMultiContentsViewDelegateImpl::BraveMultiContentsViewDelegateImpl(
    Browser& browser)
    : MultiContentsViewDelegateImpl(browser), bwi_(browser) {}

BraveMultiContentsViewDelegateImpl::~BraveMultiContentsViewDelegateImpl() =
    default;

void BraveMultiContentsViewDelegateImpl::WebContentsFocused(
    content::WebContents* contents) {
  TabStripModel* model = bwi_->tab_strip_model();
  // https://github.com/brave/brave-browser/issues/53121
  // On macOS, closing a split view detaches a web contents native view, which
  // can synchronously trigger a focus change (via AppKit first responder
  // transfer). This focus event propagates to ActivateTabAt(), but if we're
  // already inside CloseAllTabs(), the TabStripModel reentrancy guard fires.
  // Skip the activation when tabs are being closed.
  if (model->closing_all()) {
    return;
  }

  // Web panel's tab is a plain pinned tab, not part of a split, so
  // upstream's split-only activation logic never activates it when
  // its contents area is clicked/focused. Likewise, it never reactivates
  // the normal tab that gets clicked back into after the panel became
  // active. Activate directly in both cases.
  if (sidebar::IsWebPanelFeatureEnabled() &&
      sidebar::IsWebPanelRelatedFocusChange(
          bwi_->GetFeatures().sidebar_controller()->GetWebPanelController(),
          model, contents)) {
    if (tabs::TabInterface* tab =
            tabs::TabInterface::MaybeGetFromContents(contents);
        tab && model->GetActiveTab() != tab) {
      model->ActivateTabAt(model->GetIndexOfTab(tab));
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
  if (!bwi_->tab_strip_model()->GetActiveTab()->GetSplit()) {
    return;
  }

  MultiContentsViewDelegateImpl::ResizeWebContents(ratio, done_resizing);
}

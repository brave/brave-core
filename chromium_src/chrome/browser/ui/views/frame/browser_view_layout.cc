// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/frame/browser_view_layout.h"

#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"

// Double check if the tab strip is actually visible to calculate constraints.
#define SupportsWindowFeature(FEATURE)                                       \
  SupportsWindowFeature(FEATURE) && (FEATURE != Browser::FEATURE_TABSTRIP || \
                                     delegate_->ShouldDrawTabStrip());

#define BRAVE_BROWSER_VIEW_LAYOUT_CONVERTED_HIT_TEST \
  if (dst->GetWidget() != src->GetWidget()) {        \
    return false;                                    \
  }

#include "src/chrome/browser/ui/views/frame/browser_view_layout.cc"
#undef BRAVE_BROWSER_VIEW_LAYOUT_CONVERTED_HIT_TEST
#undef SupportsWindowFeature

void BrowserViewLayout::NotifyDialogPositionRequiresUpdate() {
  dialog_host_->NotifyPositionRequiresUpdate();
}

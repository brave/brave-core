/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"

#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/skia_conversions.h"

BraveTabGroupHighlight::~BraveTabGroupHighlight() = default;

SkPath BraveTabGroupHighlight::GetPath() const {
  if (auto* browser = tab_group_views_->GetBrowserWindowInterface();
      (!browser || !browser->GetFeatures()
                        .vertical_tab_controller()
                        ->ShouldShowBraveVerticalTabs()) &&
      !tabs::HorizontalTabsUpdateEnabled()) {
    return TabGroupHighlight::GetPath();
  }

  return {};
}

BEGIN_METADATA(BraveTabGroupHighlight)
END_METADATA

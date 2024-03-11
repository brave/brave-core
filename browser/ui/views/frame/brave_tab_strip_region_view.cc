/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"

#include <utility>

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view_class_properties.h"

BraveTabStripRegionView::~BraveTabStripRegionView() = default;

void BraveTabStripRegionView::Layout(PassKey) {
  UpdateBraveTabStripMargins();
  LayoutSuperclass<TabStripRegionView>(this);
}

void BraveTabStripRegionView::UpdateBraveTabStripMargins() {
  if (tabs::features::HorizontalTabsUpdateEnabled()) {
    gfx::Insets margins;
    if (!tab_strip_->controller()->IsFrameCondensed() &&
        !tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
      margins.set_left(brave_tabs::kHorizontalTabStripLeftMargin);
    }
    tab_strip_container_->SetProperty(views::kMarginsKey, margins);
  }
}

BEGIN_METADATA(BraveTabStripRegionView)
END_METADATA

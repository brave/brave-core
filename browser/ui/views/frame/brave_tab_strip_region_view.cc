/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"

#include <utility>

#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"

BraveTabStripRegionView::~BraveTabStripRegionView() = default;

void BraveTabStripRegionView::Layout(PassKey) {
  UpdateTabStripMargin();
  LayoutSuperclass<TabStripRegionView>(this);

  // Ensure that the new tab button is positioned after the last tab, with the
  // correct amount of padding.
  if (new_tab_button_) {
    new_tab_button_->SetX(tab_strip_container_->bounds().right() +
                          GetLayoutConstant(TAB_STRIP_PADDING));
  }
}

void BraveTabStripRegionView::UpdateTabStripMargin() {
  TabStripRegionView::UpdateTabStripMargin();

  gfx::Insets margins;
  bool vertical_tabs =
      tabs::utils::ShouldShowVerticalTabs(tab_strip_->GetBrowser());

  // In horizontal mode, take the current right margin. It is required so that
  // the new tab button will not be covered by the frame grab handle.
  if (!vertical_tabs) {
    if (auto* current = tab_strip_container_->GetProperty(views::kMarginsKey)) {
      margins.set_right(current->right());
    }
  }

  // Ensure that the correct amount of left margin is applied to the tabstrip.
  // When we are in a fullscreen/condensed mode, we want the tabstrip to meet
  // the frame edge so that the leftmost tab can be selected at the edge of the
  // screen.
  if (tabs::features::HorizontalTabsUpdateEnabled()) {
    if (!tab_strip_->controller()->IsFrameCondensed() && !vertical_tabs) {
      margins.set_left(brave_tabs::kHorizontalTabStripLeftMargin);
    } else {
      margins.set_left(0);
    }
  }

  tab_strip_container_->SetProperty(views::kMarginsKey, margins);
}

void BraveTabStripRegionView::Initialize() {
  // Use our own icon for the new tab button.
  if (auto* ntb = views::AsViewClass<TabStripControlButton>(new_tab_button_)) {
    ntb->SetVectorIcon(kLeoPlusAddIcon);
  }
}

BEGIN_METADATA(BraveTabStripRegionView)
END_METADATA

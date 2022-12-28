/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/z_orderable_tab_container_element.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

namespace brave {
float CalculateZValue_ChromiumImpl(views::View* child);
}

// This will replace ZOrderableTabContainer::CalculateZValue to
// brave::CalculateZValue_ChromiumImpl, which is definition for a function
// declared in the brave namespace above.
#define ZOrderableTabContainerElement brave
#define CalculateZValue CalculateZValue_ChromiumImpl
#include "src/chrome/browser/ui/views/tabs/z_orderable_tab_container_element.cc"
#undef CalculateZValue
#undef ZOrderableTabContainerElement

// static
float ZOrderableTabContainerElement::CalculateZValue(views::View* child) {
  if (views::AsViewClass<TabGroupUnderline>(child)) {
    if (auto* browser_view = BrowserView::GetBrowserViewForNativeWindow(
            child->GetWidget()->GetTopLevelWidget()->GetNativeWindow());
        browser_view &&
        tabs::features::ShouldShowVerticalTabs(browser_view->browser())) {
      // TabGroupUnderline in vertical tabs should be underneath other views.
      return 0;
    }
  }
  return brave::CalculateZValue_ChromiumImpl(child);
}

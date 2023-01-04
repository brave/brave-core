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
  // TabGroupUnderline in vertical tabs should be underneath other views.
  // Checks if the |child| is TabGroupUnderLine and its browser is showing
  // vertical tab strip.
  if (!views::AsViewClass<TabGroupUnderline>(child))
    return brave::CalculateZValue_ChromiumImpl(child);

  auto* widget = child->GetWidget();
  if (!widget)
    return brave::CalculateZValue_ChromiumImpl(child);

  auto* browser_view =
      BrowserView::GetBrowserViewForNativeWindow(widget->GetNativeWindow());
  if (!browser_view)
    return brave::CalculateZValue_ChromiumImpl(child);

  return tabs::features::ShouldShowVerticalTabs(browser_view->browser())
             ? 0
             : brave::CalculateZValue_ChromiumImpl(child);
}

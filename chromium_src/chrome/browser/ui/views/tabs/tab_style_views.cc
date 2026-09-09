/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_style_views.h"

#include <memory>
#include <utility>

#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/views/view_utils.h"

#include <chrome/browser/ui/views/tabs/tab_style_views.cc>

std::unique_ptr<TabStyleViews> TabStyleViews::Create(
    std::unique_ptr<TabStyleViewDelegate> delegate,
    TabStripOrientation orientation) {
  // Our vertical tabs are preferred when selecting, but the fallback to the
  // upstream vertical tab style is left in place too.
  if (views::AsViewClass<Tab>(delegate->GetView())) {
    return CreateBraveVerticalTabStyle(std::move(delegate));
  }
  if (orientation == TabStripOrientation::kVertical) {
    return std::make_unique<VerticalTabStyleViews>(std::move(delegate));
  }
  return std::make_unique<HorizontalTabStyleViews>(std::move(delegate));
}

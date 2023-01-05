/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_group_views.h"

BraveTabGroupHighlight::~BraveTabGroupHighlight() = default;

SkPath BraveTabGroupHighlight::GetPath() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return TabGroupHighlight::GetPath();

  if (!tabs::features::ShouldShowVerticalTabs(tab_group_views_->GetBrowser()))
    return TabGroupHighlight::GetPath();

  // We don't have to paint highlight as we have TabGroupUnderline draw a
  // enclosing box.
  return {};
}

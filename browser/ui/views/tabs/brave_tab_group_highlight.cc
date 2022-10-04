/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/features.h"

BraveTabGroupHighlight::~BraveTabGroupHighlight() = default;

SkPath BraveTabGroupHighlight::GetPath() const {
  if (!tabs::features::ShouldShowVerticalTabs())
    return TabGroupHighlight::GetPath();

  SkPath path;
  path.moveTo(BraveTabGroupHeader::GetLeftPaddingForVerticalTabs(), height());
  path.lineTo(width(), height());
  path.lineTo(width(), 0);
  path.lineTo(BraveTabGroupHeader::GetLeftPaddingForVerticalTabs(), 0);
  path.close();

  return path;
}

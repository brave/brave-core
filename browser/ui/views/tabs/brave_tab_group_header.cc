/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/label.h"

// static
int BraveTabGroupHeader::GetLeftPaddingForVerticalTabs() {
  return TabGroupUnderline::kStrokeThickness + 1;
}

BraveTabGroupHeader::~BraveTabGroupHeader() = default;

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();
  if (!tabs::features::ShouldShowVerticalTabs())
    return;

  title_chip_->SetX(GetLeftPaddingForVerticalTabs());
}

BEGIN_METADATA(BraveTabGroupHeader, TabGroupHeader)
END_METADATA

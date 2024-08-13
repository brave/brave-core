/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/public/tab_features.h"

#include "brave/browser/ui/side_panel/brave_side_panel_utils.h"

#define Init Init_ChromiumImpl
#include "src/chrome/browser/ui/tabs/tab_features.cc"
#undef Init

namespace tabs {

void TabFeatures::Init(TabInterface& tab, Profile* profile) {
  Init_ChromiumImpl(tab, profile);
  // Expect upstream's Init to create the registry.
  CHECK(side_panel_registry_.get());
  brave::RegisterContextualSidePanel(side_panel_registry_.get(),
                                     tab.GetContents());
}

}  // namespace tabs

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"
#include "ui/base/ui_base_features.h"

// When the chrome refresh flag is active, the layout logic in `TabGroupHeader`
// ignores the top insets returned from the group style's
// `GetInsetsForHeaderChip` method. Temporarily use the old code paths until
// this is addressed upstream.
namespace features {
static bool BraveIsChromeRefresh2023() {
  return false;
}
}  // namespace features

#define IsChromeRefresh2023 BraveIsChromeRefresh2023
#define TabGroupUnderline BraveTabGroupUnderline

#include "src/chrome/browser/ui/views/tabs/tab_group_header.cc"

#undef TabGroupUnderline
#undef IsChromeRefresh2023

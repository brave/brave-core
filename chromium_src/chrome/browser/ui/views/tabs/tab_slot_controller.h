/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_types.h"
#include "ui/base/models/image_model.h"

// Defined in brave/browser/ui/tabs/brave_tab_prefs.h, which this header's
// target doesn't depend on. A value-initialized TabMinWidthMode is kDefault.
namespace brave_tabs {
enum class TabMinWidthMode;
}  // namespace brave_tabs

namespace tabs {
class TreeTabNode;
}  // namespace tabs

namespace tree_tab {
class TreeTabNodeId;
}  // namespace tree_tab

#include <chrome/browser/ui/views/tabs/tab_slot_controller.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_SLOT_CONTROLLER_H_

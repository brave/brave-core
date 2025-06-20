// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_LIST_ACTION_MODIFIERS_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_LIST_ACTION_MODIFIERS_H_

#include <vector>

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar.mojom.h"

namespace customize_chrome {

std::vector<side_panel::customize_chrome::mojom::ActionPtr>
FilterUnsupportedChromiumActions(
    std::vector<side_panel::customize_chrome::mojom::ActionPtr> actions);

}  // namespace customize_chrome

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_LIST_ACTION_MODIFIERS_H_

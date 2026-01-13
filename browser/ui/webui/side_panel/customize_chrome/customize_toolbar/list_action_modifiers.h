// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_LIST_ACTION_MODIFIERS_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_LIST_ACTION_MODIFIERS_H_

#include <vector>

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar.mojom.h"

namespace content {
class WebContents;
}  // namespace content

namespace customize_chrome {

// Append Brave-specific categories to the list of categories.
//  * We have our own "Address bar" category that contains actions like
//    `kShowReward`.
std::vector<side_panel::customize_chrome::mojom::CategoryPtr>
AppendBraveSpecificCategories(
    content::WebContents& web_contents,
    std::vector<side_panel::customize_chrome::mojom::CategoryPtr> categories);

// Removes unsupported Chromium actions from the list of actions.
std::vector<side_panel::customize_chrome::mojom::ActionPtr>
FilterUnsupportedChromiumActions(
    std::vector<side_panel::customize_chrome::mojom::ActionPtr> actions);

// Applies Brave-specific modifications to the list of actions.
// 1. Moves existing Chromium actions to the desired positions.
//   e.g. Tab search action is moved to 'Navigation' category after 'New
//        Incognito Window'.
// 2. Updates icons/strings for existing actions.
//   e.g. 'New Incognito Window' action icon is updated to use the
//        `kLeoProductPrivateWindowIcon`. This icon is different from what
//        we use for App menu.
// 3. Adds Brave-specific actions.
//   e.g. In 'Navigation' category:
//        `kShowAddBookmarkButton`, `kShowSidePanel`, `kShowWallet`,
//        `kShowAIChat`, `kShowVPN`.
//        In 'Address bar' category: `kShowReward`,
//        `kShowBraveNews`, `kShowShareMenu`.
std::vector<side_panel::customize_chrome::mojom::ActionPtr>
ApplyBraveSpecificModifications(
    content::WebContents& web_contents,
    std::vector<side_panel::customize_chrome::mojom::ActionPtr> actions);

}  // namespace customize_chrome

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_TOOLBAR_LIST_ACTION_MODIFIERS_H_

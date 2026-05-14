// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SESSIONS_BRAVE_TAB_RESTORE_TREE_HELPER_H_
#define BRAVE_BROWSER_SESSIONS_BRAVE_TAB_RESTORE_TREE_HELPER_H_

#include <map>
#include <string>

class Browser;
class TabStripModel;

namespace content {
class WebContents;
}  // namespace content

// Called from BrowserLiveTabContext::GetExtraDataForTab to record the tree-tab
// position of the tab being closed. Writes kBraveTreeNodeIdKey and
// kBraveTreeParentNodeIdKey into |extra_data|.
void BravePopulateTreeTabExtraData(
    TabStripModel* tab_strip_model,
    int index,
    std::map<std::string, std::string>* extra_data);

// Called from BrowserLiveTabContext::AddRestoredTab after the tab has been
// re-inserted into the browser. Reads kBraveTreeParentNodeIdKey from
// |extra_data| and, if the parent tree node is still present in the strip,
// reparents the restored tab's TreeTabNodeTabCollection under it.
void BraveRestoreTabTreeHierarchy(
    Browser* browser,
    content::WebContents* restored_wc,
    const std::map<std::string, std::string>& extra_data);

#endif  // BRAVE_BROWSER_SESSIONS_BRAVE_TAB_RESTORE_TREE_HELPER_H_

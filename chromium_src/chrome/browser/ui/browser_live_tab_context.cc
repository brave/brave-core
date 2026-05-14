/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/sessions/brave_tab_restore_tree_helper.h"

// Injected at the end of BrowserLiveTabContext::GetExtraDataForTab, before the
// return statement. Adds kBraveTreeNodeIdKey and kBraveTreeParentNodeIdKey to
// |extra_data| so that TabRestoreService can save the tree position.
#define BRAVE_GET_EXTRA_DATA_FOR_TAB \
  BravePopulateTreeTabExtraData(&tab_strip_model_.get(), index, &extra_data);

// Injected inside BrowserLiveTabContext::AddRestoredTab, just before the final
// return, where |web_contents| is guaranteed non-null. Re-establishes the
// parent-child tree relationship for the restored tab.
#define BRAVE_ADD_RESTORED_TAB \
  BraveRestoreTabTreeHierarchy(browser, web_contents, tab.extra_data);

#include <chrome/browser/ui/browser_live_tab_context.cc>

#undef BRAVE_GET_EXTRA_DATA_FOR_TAB
#undef BRAVE_ADD_RESTORED_TAB

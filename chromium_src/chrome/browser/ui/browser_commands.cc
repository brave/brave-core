/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include <memory>
#include <optional>

#include "base/check.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "components/tab_groups/tab_group_id.h"
#include "content/public/browser/web_contents.h"

class ReadingListModel;

namespace {

// Returns true if the reload was handled by Brave (a new Tor connection was
// triggered) and the caller should return early.
bool BraveMaybeHandleReloadBypassingCache(BrowserWindowInterface* browser) {
#if BUILDFLAG(ENABLE_TOR)
  Profile* profile = browser->GetProfile();
  DCHECK(profile);
  // NewTorConnectionForSite will do hard reload after obtaining new identity
  if (profile->IsTor()) {
    brave::NewTorConnectionForSite(browser);
    return true;
  }
#endif  // BUILDFLAG(ENABLE_TOR)
  return false;
}

// Inserts |contents|, the duplicate of the tab at |source_index|, at
// |dst_index|. ADD_INHERIT_OPENER makes the active tab the opener of the new
// tab, which is unrelated to the duplicated tab when duplicating a non-active
// tab (e.g. via the tab context menu). In tree tab mode, set the tab being
// duplicated as the opener instead, so that the new tab is placed in the
// source tab's tree. This is limited to tree tab mode to keep the upstream
// behavior, which tab gets activated after closing the duplicated tab, intact.
int BraveInsertDuplicatedTab(TabStripModel* tab_strip_model,
                             int source_index,
                             int dst_index,
                             std::unique_ptr<content::WebContents> contents,
                             int add_types,
                             std::optional<tab_groups::TabGroupId> group) {
  auto tab =
      std::make_unique<tabs::TabModel>(std::move(contents), tab_strip_model);
  if (static_cast<BraveTabStripModel*>(tab_strip_model)->tree_model() &&
      source_index != tab_strip_model->active_index()) {
    add_types &= ~AddTabTypes::ADD_INHERIT_OPENER;
    tab->set_opener(tab_strip_model->GetTabAtIndex(source_index));
  }
  return tab_strip_model->InsertDetachedTabAt(dst_index, std::move(tab),
                                              add_types, group);
}

}  // namespace

#include <chrome/browser/ui/browser_commands.cc>

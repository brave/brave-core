/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <vector>

#include "base/no_destructor.h"
#include "brave/browser/sessions/brave_session_tree_tab_helper.h"
#include "brave/browser/sessions/brave_tree_tab_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_service_commands.h"
#include "brave/components/constants/pref_names.h"

// File-scope storage for per-browser BraveSessionTreeTabHelper instances.
// Keyed by the owning SessionService pointer. Helpers become inert (browser_
// set to nullptr) when their TabStripModel is destroyed; they are fully
// removed when the SessionService itself is destroyed.
namespace {
using BraveTreeTabHelperMap =
    std::map<SessionService*,
             std::vector<std::unique_ptr<BraveSessionTreeTabHelper>>>;
BraveTreeTabHelperMap& GetBraveTreeTabHelpers() {
  static base::NoDestructor<BraveTreeTabHelperMap> map;
  return *map;
}
}  // namespace

// Prevent detached tab has unnecessary tab restore steps.
// When last window's last tab is closed, |has_open_trackable_browsers_|
// becomes false. If we turn off "Close window when closing last tab" option,
// another new tab is created after last tab is closed. So, it's not the last
// actually. It could make ShouldRestore() give true when creating
// new browser by detaching a tab.
#define BRAVE_SESSION_SERVICE_TAB_CLOSED \
  if (profile()->GetPrefs()->GetBoolean(kEnableClosingLastTab))

// Appended at the end of SessionService::BuildCommandsForTab to persist the
// current tree node membership and metadata for |tab| (a WebContents*).
// |session_id| and |command_storage_manager()| are already in scope.
#define BRAVE_SESSION_SERVICE_BUILD_COMMANDS_FOR_TAB                           \
  do {                                                                          \
    if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {                   \
      Browser* brave_tab_browser = chrome::FindBrowserWithTab(tab);            \
      if (brave_tab_browser) {                                                  \
        auto* brave_tsm = static_cast<BraveTabStripModel*>(                    \
            brave_tab_browser->tab_strip_model());                              \
        if (brave_tsm->tree_model()) {                                          \
          auto* tab_iface = brave_tsm->GetTabForWebContents(tab);              \
          if (tab_iface) {                                                      \
            const tabs::TabCollection* parent =                                 \
                tab_iface->GetParentCollection();                               \
            while (parent &&                                                    \
                   parent->type() != tabs::TabCollection::Type::TREE_NODE) {   \
              if (parent->type() == tabs::TabCollection::Type::UNPINNED ||     \
                  parent->type() == tabs::TabCollection::Type::PINNED ||       \
                  parent->type() == tabs::TabCollection::Type::TABSTRIP) {     \
                parent = nullptr;                                               \
                break;                                                          \
              }                                                                 \
              parent = parent->GetParentCollection();                           \
            }                                                                   \
            if (parent &&                                                       \
                parent->type() == tabs::TabCollection::Type::TREE_NODE) {      \
              auto* tree_coll =                                                 \
                  static_cast<const tabs::TreeTabNodeTabCollection*>(parent);  \
              const auto& node = tree_coll->node();                            \
              auto parent_id = node.GetParentTreeNodeId();                     \
              bool is_primary_tab =                                             \
                  tree_coll->current_value_type() ==                           \
                  tabs::TreeTabNodeTabCollection::CurrentValueType::kTab;      \
              command_storage_manager()->AppendRebuildCommand(                 \
                  sessions::CreateAddTabExtraDataCommand(                      \
                      session_id, kBraveTreeNodeIdKey,                         \
                      node.id().ToString()));                                 \
              command_storage_manager()->AppendRebuildCommand(                 \
                  sessions::CreateAddTabExtraDataCommand(                      \
                      session_id, kBraveTreeParentNodeIdKey,                   \
                      parent_id ? parent_id->ToString() : ""));               \
              if (is_primary_tab) {                                             \
                command_storage_manager()->AppendRebuildCommand(               \
                    sessions::CreateAddTabExtraDataCommand(                    \
                        session_id, kBraveTreeNodeCollapsedKey,                \
                        node.collapsed() ? "1" : "0"));                       \
              }                                                                 \
            }                                                                   \
          }                                                                     \
        }                                                                       \
      }                                                                         \
    }                                                                           \
  } while (false)

// Called at the end of SessionService::WindowOpened (browser still alive,
// already tracked). Creates a BraveSessionTreeTabHelper for the browser if
// tree tabs are active so that incremental tree-structure changes (new child
// tabs, collapsed-state changes) are saved via ScheduleCommand.
#define BRAVE_SESSION_SERVICE_WINDOW_OPENED                                    \
  do {                                                                          \
    if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {                   \
      auto* brave_tsm =                                                         \
          static_cast<BraveTabStripModel*>(browser->tab_strip_model());        \
      if (brave_tsm->tree_model()) {                                            \
        GetBraveTreeTabHelpers()[this].push_back(                              \
            std::make_unique<BraveSessionTreeTabHelper>(browser, this));       \
      }                                                                         \
    }                                                                           \
  } while (false)

// Called at the start of SessionService::WindowClosing (browser is still in
// the browser list and its tab strip is intact). Forces a full session rebuild
// so that any tree-structure reparenting that happened since the last periodic
// rebuild is captured before the browser is removed from the list.
#define BRAVE_SESSION_SERVICE_WINDOW_CLOSING                                   \
  do {                                                                          \
    if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {                   \
      ScheduleResetCommands();                                                  \
    }                                                                           \
  } while (false)

// Called at the start of SessionService::~SessionService to release all
// BraveSessionTreeTabHelper instances owned by this service.
#define BRAVE_SESSION_SERVICE_DESTRUCTOR \
  GetBraveTreeTabHelpers().erase(this)

#include <chrome/browser/sessions/session_service.cc>

#undef BRAVE_SESSION_SERVICE_TAB_CLOSED
#undef BRAVE_SESSION_SERVICE_BUILD_COMMANDS_FOR_TAB
#undef BRAVE_SESSION_SERVICE_WINDOW_OPENED
#undef BRAVE_SESSION_SERVICE_WINDOW_CLOSING
#undef BRAVE_SESSION_SERVICE_DESTRUCTOR

// ---- Brave additions after the Chromium include ----

void SessionService::SetTreeTabNodeData(SessionID window_id,
                                        SessionID tab_id,
                                        const std::string& node_id,
                                        const std::string& parent_node_id,
                                        bool collapsed,
                                        bool has_collapsed_key) {
  if (!ShouldTrackChangesToWindow(window_id)) {
    return;
  }
  ScheduleCommand(sessions::CreateAddTabExtraDataCommand(
      tab_id, kBraveTreeNodeIdKey, node_id));
  ScheduleCommand(sessions::CreateAddTabExtraDataCommand(
      tab_id, kBraveTreeParentNodeIdKey, parent_node_id));
  if (has_collapsed_key) {
    ScheduleCommand(sessions::CreateAddTabExtraDataCommand(
        tab_id, kBraveTreeNodeCollapsedKey, collapsed ? "1" : "0"));
  }
}


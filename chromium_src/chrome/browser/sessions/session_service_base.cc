/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/sessions/brave_tree_tab_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/sessions/content/session_tab_helper.h"

// Appended inside the per-tab loop of SessionServiceBase::BuildCommandsForBrowser,
// right after BuildCommandsForTab. |tab_interface| (tabs::TabInterface*) and
// |browser| (Browser*) are in scope, as is command_storage_manager().
// Mirrors the group/split handling in the same loop.
#define BRAVE_BUILD_COMMANDS_FOR_TREE_TAB                                      \
  do {                                                                          \
    if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {                   \
      auto* brave_tsm = static_cast<BraveTabStripModel*>(                      \
          browser->tab_strip_model());                                          \
      if (brave_tsm->tree_model()) {                                            \
        const tabs::TabCollection* parent =                                     \
            tab_interface->GetParentCollection();                               \
        while (parent &&                                                        \
               parent->type() != tabs::TabCollection::Type::TREE_NODE) {       \
          if (parent->type() == tabs::TabCollection::Type::UNPINNED ||         \
              parent->type() == tabs::TabCollection::Type::PINNED ||           \
              parent->type() == tabs::TabCollection::Type::TABSTRIP) {         \
            parent = nullptr;                                                   \
            break;                                                              \
          }                                                                     \
          parent = parent->GetParentCollection();                               \
        }                                                                       \
        if (parent &&                                                           \
            parent->type() == tabs::TabCollection::Type::TREE_NODE) {          \
          auto* tree_coll =                                                     \
              static_cast<const tabs::TreeTabNodeTabCollection*>(parent);       \
          const auto& node = tree_coll->node();                                 \
          auto parent_id = node.GetParentTreeNodeId();                          \
          bool is_primary_tab =                                                  \
              tree_coll->current_value_type() ==                                \
              tabs::TreeTabNodeTabCollection::CurrentValueType::kTab;           \
          auto* session_helper = sessions::SessionTabHelper::FromWebContents(   \
              tab_interface->GetContents());                                     \
          if (session_helper) {                                                  \
            SessionID tab_session_id = session_helper->session_id();            \
            command_storage_manager()->AppendRebuildCommand(                    \
                sessions::CreateAddTabExtraDataCommand(                         \
                    tab_session_id, kBraveTreeNodeIdKey,                        \
                    node.id().ToString()));                                      \
            command_storage_manager()->AppendRebuildCommand(                    \
                sessions::CreateAddTabExtraDataCommand(                         \
                    tab_session_id, kBraveTreeParentNodeIdKey,                  \
                    parent_id ? parent_id->ToString() : ""));                   \
            if (is_primary_tab) {                                               \
              command_storage_manager()->AppendRebuildCommand(                  \
                  sessions::CreateAddTabExtraDataCommand(                       \
                      tab_session_id, kBraveTreeNodeCollapsedKey,               \
                      node.collapsed() ? "1" : "0"));                          \
            }                                                                   \
          }                                                                     \
        }                                                                       \
      }                                                                         \
    }                                                                           \
  } while (false)

#include <chrome/browser/sessions/session_service_base.cc>

#undef BRAVE_BUILD_COMMANDS_FOR_TREE_TAB

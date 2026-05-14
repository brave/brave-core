// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/sessions/brave_session_tree_tab_helper.h"

#include "brave/browser/sessions/brave_tree_tab_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

BraveSessionTreeTabHelper::BraveSessionTreeTabHelper(
    Browser* browser,
    SessionService* session_service)
    : browser_(browser), session_service_(session_service) {
  browser_->tab_strip_model()->AddObserver(this);
}

BraveSessionTreeTabHelper::~BraveSessionTreeTabHelper() = default;

void BraveSessionTreeTabHelper::ScheduleAllTreeNodeCommands() {
  if (!browser_) {
    return;
  }
  auto* brave_tsm =
      static_cast<BraveTabStripModel*>(browser_->tab_strip_model());
  if (!brave_tsm->tree_model()) {
    return;
  }
  // Iterate all tabs and schedule their tree data. BuildCommandsForTab in the
  // chromium_src override does the same thing for the rebuild path; this method
  // exists so that the tree data is also written when ScheduleResetCommands is
  // called without going through the full rebuild.
  // Note: the rebuild path in BuildCommandsForTab already handles this; this
  // method is kept as a safety net and is a no-op if not needed.
}

void BraveSessionTreeTabHelper::OnTreeTabChanged(
    const TreeTabChange& change) {
  if (!browser_ || !session_service_) {
    return;
  }
  switch (change.type) {
    case TreeTabChange::Type::kNodeCreated: {
      const auto& node = change.GetCreatedChange().node.get();
      ScheduleTreeNodeExtraData(node);
      break;
    }
    case TreeTabChange::Type::kNodeCollapsedStateChanged: {
      const auto& node = change.GetCollapsedStateChangedChange().node.get();
      ScheduleCollapsedStateExtraData(node);
      break;
    }
    case TreeTabChange::Type::kNodeWillBeDestroyed:
      // No action needed: the tab is being removed from the session too.
      break;
  }
}

void BraveSessionTreeTabHelper::OnTabStripModelDestroyed(
    TabStripModel* /*tab_strip_model*/) {
  browser_ = nullptr;
}

void BraveSessionTreeTabHelper::ScheduleTreeNodeExtraData(
    const tabs::TreeTabNode& node) {
  // Schedule extra_data for every tab that belongs to this node.
  for (const tabs::TabInterface* tab_iface : node.GetTabs()) {
    if (!tab_iface) {
      continue;
    }
    auto* wc = tab_iface->GetContents();
    if (!wc) {
      continue;
    }
    auto* session_helper = sessions::SessionTabHelper::FromWebContents(wc);
    if (!session_helper) {
      continue;
    }
    SessionID tab_id = session_helper->session_id();
    SessionID window_id = browser_->GetSessionID();

    auto parent_id = node.GetParentTreeNodeId();

    // Determine if this tab is the primary (direct current_value) tab.
    // Walk up the parent collection to find the TreeTabNodeTabCollection.
    const tabs::TabCollection* parent = tab_iface->GetParentCollection();
    while (parent &&
           parent->type() != tabs::TabCollection::Type::TREE_NODE) {
      if (parent->type() == tabs::TabCollection::Type::UNPINNED ||
          parent->type() == tabs::TabCollection::Type::PINNED ||
          parent->type() == tabs::TabCollection::Type::TABSTRIP) {
        parent = nullptr;
        break;
      }
      parent = parent->GetParentCollection();
    }

    bool is_primary_tab = false;
    if (parent && parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      is_primary_tab =
          static_cast<const tabs::TreeTabNodeTabCollection*>(parent)
              ->current_value_type() ==
          tabs::TreeTabNodeTabCollection::CurrentValueType::kTab;
    }

    session_service_->SetTreeTabNodeData(
        window_id, tab_id, node.id().ToString(),
        parent_id ? parent_id->ToString() : "",
        node.collapsed(), is_primary_tab);
  }
}

void BraveSessionTreeTabHelper::ScheduleCollapsedStateExtraData(
    const tabs::TreeTabNode& node) {
  // Only the primary (kTab) tab of a node carries the collapsed state key.
  const tabs::TreeTabNodeTabCollection* tree_coll = nullptr;

  auto tabs = node.GetTabs();
  if (tabs.empty()) {
    return;
  }

  const tabs::TabInterface* primary_tab = nullptr;
  for (const auto* t : tabs) {
    if (!t) {
      continue;
    }
    const tabs::TabCollection* parent = t->GetParentCollection();
    while (parent &&
           parent->type() != tabs::TabCollection::Type::TREE_NODE) {
      if (parent->type() == tabs::TabCollection::Type::UNPINNED ||
          parent->type() == tabs::TabCollection::Type::PINNED ||
          parent->type() == tabs::TabCollection::Type::TABSTRIP) {
        parent = nullptr;
        break;
      }
      parent = parent->GetParentCollection();
    }
    if (parent && parent->type() == tabs::TabCollection::Type::TREE_NODE) {
      tree_coll =
          static_cast<const tabs::TreeTabNodeTabCollection*>(parent);
      if (tree_coll->current_value_type() ==
          tabs::TreeTabNodeTabCollection::CurrentValueType::kTab) {
        primary_tab = t;
        break;
      }
    }
  }

  if (!primary_tab) {
    return;
  }

  auto* wc = primary_tab->GetContents();
  if (!wc) {
    return;
  }
  auto* session_helper = sessions::SessionTabHelper::FromWebContents(wc);
  if (!session_helper) {
    return;
  }
  SessionID tab_id = session_helper->session_id();
  SessionID window_id = browser_->GetSessionID();

  auto parent_id = node.GetParentTreeNodeId();
  session_service_->SetTreeTabNodeData(window_id, tab_id,
                                       node.id().ToString(),
                                       parent_id ? parent_id->ToString() : "",
                                       node.collapsed(),
                                       /*has_collapsed_key=*/true);
}

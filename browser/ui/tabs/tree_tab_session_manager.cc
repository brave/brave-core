// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/tree_tab_session_manager.h"

#include <map>
#include <string>

#include "base/check.h"
#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/browser/sessions/brave_session_keys.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_id.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

TreeTabSessionManager::TreeTabSessionManager(Profile* profile,
                                             TabStripModel* tab_strip_model,
                                             SessionID session_id)
    : profile_(profile),
      tab_strip_model_(tab_strip_model),
      session_id_(session_id) {
  CHECK(profile_);
  CHECK(tab_strip_model_);
  tab_strip_model_->AddObserver(this);
}

TreeTabSessionManager::~TreeTabSessionManager() = default;

void TreeTabSessionManager::MaybePopulateTreeTabExtraData(
    int index,
    std::map<std::string, std::string>* extra_data) {
  auto* brave_tab_strip_model =
      static_cast<BraveTabStripModel*>(tab_strip_model_);
  if (!brave_tab_strip_model->tree_model()) {
    // Can be null if current tab strip doesn't show tab strip as tree
    // structure.
    return;
  }

  auto* tab_interface = brave_tab_strip_model->GetTabAtIndex(index);
  CHECK(tab_interface);

  const auto* tree_collection =
      tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tab_interface);
  if (!tree_collection) {
    // In case the tab is under group or split, tree_collection can be null.
    // TODO(https://github.com/brave/brave-browser/issues/49792): Add support
    // for tabs in groups or splits.
    return;
  }

  (*extra_data)[kBraveTreeNodeIdKey] = tree_collection->node().id().ToString();
  auto parent_id = tree_collection->node().GetParentTreeNodeId();
  (*extra_data)[kBraveTreeParentNodeIdKey] =
      parent_id ? parent_id->ToString() : "";
  (*extra_data)[kBraveTreeNodeCollapsedKey] =
      tree_collection->node().collapsed() ? "1" : "0";
}

void TreeTabSessionManager::MaybeRestoreTabTreeHierarchy(
    content::WebContents* restored_web_contents,
    const std::map<std::string, std::string>& extra_data) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return;
  }

  auto* brave_tab_strip_model =
      static_cast<BraveTabStripModel*>(tab_strip_model_);
  if (!brave_tab_strip_model->tree_model()) {
    return;
  }

  auto* node_id = base::FindOrNull(extra_data, kBraveTreeNodeIdKey);
  if (!node_id || node_id->empty()) {
    // Can be pinned tab.
    return;
  }

  // Locate the restored tab in the strip.
  auto* restored_tab =
      brave_tab_strip_model->GetTabForWebContents(restored_web_contents);
  CHECK(restored_tab);

  tabs::TreeTabNodeTabCollection* child_collection =
      tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(restored_tab);

  auto token = base::Token::FromString(*node_id);
  CHECK(token);
  child_collection->SetNodeId(
      tree_tab::TreeTabNodeId::FromRawToken(token.value()));

  // Only reparent if the saved data describes a child node (non-empty parent).
  auto* parent_id = base::FindOrNull(extra_data, kBraveTreeParentNodeIdKey);
  if (!parent_id || parent_id->empty()) {
    return;
  }
  const std::string& target_parent_node_id = *parent_id;

  if (!child_collection) {
    // Not a child of tree node tab.
    // TODO(https://github.com/brave/brave-browser/issues/49792) Consider
    // group and split.
    return;
  }

  // Scan the strip to find the live TreeTabNodeTabCollection whose node ID
  // matches the saved parent node ID. Because the parent tab was not closed
  // (only the child was), its node ID is stable within this session.
  tabs::TreeTabNodeTabCollection* parent_collection = nullptr;
  for (int i = 0; i < brave_tab_strip_model->count(); ++i) {
    auto* candidate = brave_tab_strip_model->GetTabAtIndex(i);
    if (!candidate || candidate == restored_tab) {
      continue;
    }
    const auto* coll =
        tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(candidate);
    if (!coll) {
      continue;
    }
    if (coll->node().id().ToString() == target_parent_node_id) {
      parent_collection = const_cast<tabs::TreeTabNodeTabCollection*>(coll);
      break;
    }
  }

  if (!parent_collection) {
    return;  // Parent tab was also closed; nothing to reparent under.
  }

  if (child_collection->GetParentCollection() == parent_collection) {
    return;  // Already correctly nested (shouldn't happen, but guard anyway).
  }

  auto* current_parent = child_collection->GetParentCollection();
  auto owned = current_parent->MaybeRemoveCollection(child_collection);
  parent_collection->AddCollection(std::move(owned),
                                   parent_collection->ChildCount());
}

void TreeTabSessionManager::OnTreeTabChanged(const TreeTabChange& change) {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveTreeTab));

  switch (change.type) {
    case TreeTabChange::Type::kNodeCreated:
      UpdateTreeTabSessionDataForNode(change.GetCreatedChange().node.get());
      break;
    case TreeTabChange::Type::kNodeCollapsedStateChanged:
      UpdateTreeTabCollapsedState(
          change.GetCollapsedStateChangedChange().node.get());
      break;
    case TreeTabChange::Type::kNodeReparented:
      UpdateTreeTabSessionDataForNode(change.GetReparentedChange().node.get());
      break;
    case TreeTabChange::Type::kNodeWillBeDestroyed:
      // Do not clear tree data when a node is destroyed. The tree node data
      // must remain in the session so that "restore closed tab" can place the
      // tab back into its original tree position. When the tab is actually
      // closed, SessionService::TabClosed() removes the whole tab from the
      // session. On browser restart a full rebuild will naturally omit tree
      // data for any tabs that are no longer in tree nodes.
      break;
  }
}

void TreeTabSessionManager::UpdateTreeTabSessionDataForNode(
    const tabs::TreeTabNode& node) {
  SessionService* const session_service =
      SessionServiceFactory::GetForProfileIfExisting(profile_);
  if (!session_service) {
    return;
  }

  auto parent_id = node.GetParentTreeNodeId();
  const std::string parent_id_str = parent_id ? parent_id->ToString() : "";
  const std::string node_id_str = node.id().ToString();

  for (const tabs::TabInterface* tab_iface : node.GetTabs()) {
    auto* wc = tab_iface->GetContents();
    CHECK(wc);
    auto* session_helper = sessions::SessionTabHelper::FromWebContents(wc);
    CHECK(session_helper);

    const SessionID tab_id = session_helper->session_id();
    const auto* tree_coll =
        tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tab_iface);
    if (!tree_coll) {
      // TODO(https://github.com/brave/brave-browser/issues/49792): Add support
      // Tabs are in the groups or splits. In this case, we don't do anything
      // for now.
      continue;
    }

    CHECK_EQ(tree_coll->current_value_type(),
             tabs::TreeTabNodeTabCollection::CurrentValueType::kTab);
    session_service->AddTabExtraData(session_id_, tab_id, kBraveTreeNodeIdKey,
                                     node_id_str);
    session_service->AddTabExtraData(session_id_, tab_id,
                                     kBraveTreeParentNodeIdKey, parent_id_str);
    session_service->AddTabExtraData(session_id_, tab_id,
                                     kBraveTreeNodeCollapsedKey,
                                     node.collapsed() ? "1" : "0");
  }
}

void TreeTabSessionManager::UpdateTreeTabCollapsedState(
    const tabs::TreeTabNode& node) {
  SessionService* const session_service =
      SessionServiceFactory::GetForProfileIfExisting(profile_);
  if (!session_service) {
    return;
  }

  // Only the primary (kTab) tab of a node carries the collapsed state key.
  for (const tabs::TabInterface* tab_iface : node.GetTabs()) {
    const auto* tree_coll =
        tabs::TreeTabNodeTabCollection::GetTreeTabNodeCollection(tab_iface);
    if (!tree_coll) {
      // TODO(https://github.com/brave/brave-browser/issues/49792): Add support
      // Tabs are in the groups or splits. In this case, we don't do anything
      // for now.
      continue;
    }

    auto* wc = tab_iface->GetContents();
    CHECK(wc);

    auto* session_helper = sessions::SessionTabHelper::FromWebContents(wc);
    CHECK(session_helper);

    session_service->AddTabExtraData(session_id_, session_helper->session_id(),
                                     kBraveTreeNodeCollapsedKey,
                                     node.collapsed() ? "1" : "0");
  }
}

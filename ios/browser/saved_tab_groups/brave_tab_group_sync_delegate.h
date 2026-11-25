// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_DELEGATE_H_
#define BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_DELEGATE_H_

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "base/uuid.h"
#include "components/saved_tab_groups/delegate/tab_group_sync_delegate.h"
#include "components/saved_tab_groups/public/saved_tab_group.h"
#include "components/saved_tab_groups/public/types.h"

@protocol BraveTabGroupSyncLocalModel;

namespace brave {

// Reads local groups for tab group sync and ignores remote applies.
// `TabGroupLocalUpdateObserver` is not installed: it CHECKs that no regular
// browser exists, and Brave's tabs are not in a `WebStateList`.
class BraveTabGroupSyncDelegate : public tab_groups::TabGroupSyncDelegate {
 public:
  explicit BraveTabGroupSyncDelegate(id<BraveTabGroupSyncLocalModel> model);
  BraveTabGroupSyncDelegate(const BraveTabGroupSyncDelegate&) = delete;
  BraveTabGroupSyncDelegate& operator=(const BraveTabGroupSyncDelegate&) =
      delete;
  ~BraveTabGroupSyncDelegate() override;

  std::unique_ptr<ScopedBatchOperation> StartBatchOperation() override;
  std::optional<tab_groups::LocalTabGroupID> HandleOpenTabGroupRequest(
      const base::Uuid& sync_tab_group_id,
      std::unique_ptr<tab_groups::TabGroupActionContext> context) override;
  std::unique_ptr<tab_groups::ScopedLocalObservationPauser>
  CreateScopedLocalObserverPauser() override;
  void CreateLocalTabGroup(const tab_groups::SavedTabGroup& tab_group) override;
  void CloseLocalTabGroup(const tab_groups::LocalTabGroupID& local_id) override;
  void ConnectLocalTabGroup(const tab_groups::SavedTabGroup& group) override;
  void DisconnectLocalTabGroup(
      const tab_groups::LocalTabGroupID& local_id) override;
  void UpdateLocalTabGroup(const tab_groups::SavedTabGroup& group) override;
  std::vector<tab_groups::LocalTabGroupID> GetLocalTabGroupIds() override;
  std::vector<tab_groups::LocalTabID> GetLocalTabIdsForTabGroup(
      const tab_groups::LocalTabGroupID& local_tab_group_id) override;
  std::set<tab_groups::LocalTabID> GetSelectedTabs() override;
  std::u16string GetTabTitle(
      const tab_groups::LocalTabID& local_tab_id) override;
  std::unique_ptr<tab_groups::SavedTabGroup> CreateSavedTabGroupFromLocalGroup(
      const tab_groups::LocalTabGroupID& local_tab_group_id) override;

 private:
  id<BraveTabGroupSyncLocalModel> model_;
};

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_SAVED_TAB_GROUPS_BRAVE_TAB_GROUP_SYNC_DELEGATE_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/saved_tab_groups/brave_tab_group_sync_delegate.h"

#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/strings/sys_string_conversions.h"
#include "base/synchronization/lock.h"
#import "brave/ios/browser/saved_tab_groups/brave_tab_group_sync_model.h"
#include "brave/ios/browser/saved_tab_groups/tab_group_id.h"
#include "components/saved_tab_groups/public/saved_tab_group_tab.h"
#include "components/saved_tab_groups/public/utils.h"
#include "components/tab_groups/tab_group_color.h"
#include "url/gurl.h"

namespace brave {
namespace {

class LocalObservationPauser : public tab_groups::ScopedLocalObservationPauser {
 public:
  explicit LocalObservationPauser(id<BraveTabGroupSyncLocalModel> model)
      : model_(model) {
    [model_ setLocalObservationPaused:YES];
  }

  ~LocalObservationPauser() override { [model_ setLocalObservationPaused:NO]; }

 private:
  id<BraveTabGroupSyncLocalModel> model_;
};

NSUUID* UuidForGroup(const tab_groups::LocalTabGroupID& group_id) {
  uint8_t bytes[16];
  UuidBytesFromTabGroupId(group_id, base::span<uint8_t, 16>(bytes));
  return [[NSUUID alloc] initWithUUIDBytes:bytes];
}

std::optional<tab_groups::LocalTabGroupID> GroupForUuid(NSUUID* uuid) {
  if (!uuid) {
    return std::nullopt;
  }
  uint8_t bytes[16];
  [uuid getUUIDBytes:bytes];
  return TabGroupIdFromUuidBytes(base::span<const uint8_t, 16>(bytes));
}

tab_groups::TabGroupColorId ColorFromModel(int16_t color) {
  if (color < 0 ||
      color >= static_cast<int16_t>(tab_groups::TabGroupColorId::kNumEntries)) {
    return tab_groups::TabGroupColorId::kGrey;
  }
  return static_cast<tab_groups::TabGroupColorId>(color);
}

std::vector<int> SyncTabIDs(NSArray<NSNumber*>* numbers) {
  std::vector<int> ids;
  for (NSNumber* number in numbers) {
    const int sync_tab_id = number.intValue;
    if (sync_tab_id > 0) {
      ids.push_back(sync_tab_id);
    }
  }
  return ids;
}

}  // namespace

BraveTabGroupSyncDelegate::BraveTabGroupSyncDelegate(
    id<BraveTabGroupSyncLocalModel> model)
    : model_(model) {}
BraveTabGroupSyncDelegate::~BraveTabGroupSyncDelegate() = default;

std::unique_ptr<tab_groups::TabGroupSyncDelegate::ScopedBatchOperation>
BraveTabGroupSyncDelegate::StartBatchOperation() {
  return std::make_unique<ScopedBatchOperation>();
}

std::optional<tab_groups::LocalTabGroupID>
BraveTabGroupSyncDelegate::HandleOpenTabGroupRequest(
    const base::Uuid& sync_tab_group_id,
    std::unique_ptr<tab_groups::TabGroupActionContext> context) {
  return std::nullopt;
}

std::unique_ptr<tab_groups::ScopedLocalObservationPauser>
BraveTabGroupSyncDelegate::CreateScopedLocalObserverPauser() {
  return std::make_unique<LocalObservationPauser>(model_);
}

void BraveTabGroupSyncDelegate::CreateLocalTabGroup(
    const tab_groups::SavedTabGroup& tab_group) {}

void BraveTabGroupSyncDelegate::CloseLocalTabGroup(
    const tab_groups::LocalTabGroupID& local_id) {}

void BraveTabGroupSyncDelegate::ConnectLocalTabGroup(
    const tab_groups::SavedTabGroup& group) {}

void BraveTabGroupSyncDelegate::DisconnectLocalTabGroup(
    const tab_groups::LocalTabGroupID& local_id) {}

void BraveTabGroupSyncDelegate::UpdateLocalTabGroup(
    const tab_groups::SavedTabGroup& group) {}

std::vector<tab_groups::LocalTabGroupID>
BraveTabGroupSyncDelegate::GetLocalTabGroupIds() {
  std::vector<tab_groups::LocalTabGroupID> group_ids;
  for (NSUUID* uuid in [model_ localGroupIDs]) {
    if (std::optional<tab_groups::LocalTabGroupID> group_id =
            GroupForUuid(uuid)) {
      group_ids.push_back(*group_id);
    }
  }
  return group_ids;
}

std::vector<tab_groups::LocalTabID>
BraveTabGroupSyncDelegate::GetLocalTabIdsForTabGroup(
    const tab_groups::LocalTabGroupID& local_tab_group_id) {
  return SyncTabIDs(
      [model_ syncTabIDsForGroupID:UuidForGroup(local_tab_group_id)]);
}

std::set<tab_groups::LocalTabID> BraveTabGroupSyncDelegate::GetSelectedTabs() {
  std::set<tab_groups::LocalTabID> selected;
  for (const int sync_tab_id : SyncTabIDs([model_ selectedSyncTabIDs])) {
    selected.insert(sync_tab_id);
  }
  return selected;
}

std::u16string BraveTabGroupSyncDelegate::GetTabTitle(
    const tab_groups::LocalTabID& local_tab_id) {
  if (local_tab_id <= 0) {
    return std::u16string();
  }
  return base::SysNSStringToUTF16(
      [model_ titleForSyncTabID:static_cast<int32_t>(local_tab_id)]);
}

std::unique_ptr<tab_groups::SavedTabGroup>
BraveTabGroupSyncDelegate::CreateSavedTabGroupFromLocalGroup(
    const tab_groups::LocalTabGroupID& local_tab_group_id) {
  NSUUID* group_uuid = UuidForGroup(local_tab_group_id);
  const std::vector<int> sync_tab_ids =
      SyncTabIDs([model_ syncTabIDsForGroupID:group_uuid]);
  if (sync_tab_ids.empty()) {
    return nullptr;
  }

  // Keeps the local-observation flag set while the saved group is built.
  std::unique_ptr<LocalObservationPauser> pauser =
      std::make_unique<LocalObservationPauser>(model_);
  (void)pauser;
  const base::Uuid saved_guid = base::Uuid::GenerateRandomV4();
  std::vector<tab_groups::SavedTabGroupTab> saved_tabs;
  saved_tabs.reserve(sync_tab_ids.size());
  for (size_t index = 0; index < sync_tab_ids.size(); ++index) {
    const int32_t sync_tab_id = static_cast<int32_t>(sync_tab_ids[index]);
    NSURL* url = [model_ URLForSyncTabID:sync_tab_id];
    GURL gurl =
        url ? GURL(base::SysNSStringToUTF8(url.absoluteString)) : GURL();
    std::u16string title =
        base::SysNSStringToUTF16([model_ titleForSyncTabID:sync_tab_id]);
    if (!gurl.is_valid()) {
      std::pair<GURL, std::u16string> fallback =
          tab_groups::GetDefaultUrlAndTitle();
      gurl = std::move(fallback.first);
      title = std::move(fallback.second);
    }
    saved_tabs.emplace_back(gurl, title, saved_guid, std::make_optional(index),
                            /*saved_tab_guid=*/std::nullopt, sync_tab_id);
  }

  return std::make_unique<tab_groups::SavedTabGroup>(
      base::SysNSStringToUTF16([model_ titleForGroupID:group_uuid]),
      ColorFromModel([model_ colorForGroupID:group_uuid]), saved_tabs,
      /*position=*/std::nullopt, saved_guid, local_tab_group_id);
}

}  // namespace brave

namespace {

base::Lock& ModelLock() {
  static base::NoDestructor<base::Lock> lock;
  return *lock;
}

id<BraveTabGroupSyncLocalModel> g_model = nil;

}  // namespace

@implementation BraveTabGroupSyncLocalModels

+ (void)setSharedModel:(id<BraveTabGroupSyncLocalModel>)model {
  base::AutoLock guard(ModelLock());
  g_model = model;
}

+ (id<BraveTabGroupSyncLocalModel>)sharedModel {
  base::AutoLock guard(ModelLock());
  return g_model;
}

@end

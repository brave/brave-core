// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/browser/saved_tab_groups/brave_tab_group_sync_bridge.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/strings/sys_string_conversions.h"
#include "base/uuid.h"
#include "brave/ios/browser/saved_tab_groups/brave_tab_group_sync_delegate.h"
#include "brave/ios/browser/saved_tab_groups/brave_tab_group_sync_model.h"
#include "brave/ios/browser/saved_tab_groups/saved_tab_url.h"
#include "brave/ios/browser/saved_tab_groups/tab_group_id.h"
#include "brave/ios/browser/saved_tab_groups/tab_group_sync_bridge.h"
#include "components/saved_tab_groups/public/saved_tab_group.h"
#include "components/saved_tab_groups/public/saved_tab_group_tab.h"
#include "components/saved_tab_groups/public/tab_group_sync_service.h"
#include "components/tab_groups/tab_group_color.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "url/gurl.h"

namespace brave {
namespace {

tab_groups::TabGroupColorId ColorFromModel(int16_t color) {
  if (color < 0 ||
      color >= static_cast<int16_t>(tab_groups::TabGroupColorId::kNumEntries)) {
    return tab_groups::TabGroupColorId::kGrey;
  }
  return static_cast<tab_groups::TabGroupColorId>(color);
}

std::optional<tab_groups::LocalTabGroupID> LocalId(NSUUID* uuid) {
  if (!uuid) {
    return std::nullopt;
  }
  uint8_t bytes[16];
  [uuid getUUIDBytes:bytes];
  return TabGroupIdFromUuidBytes(base::span<const uint8_t, 16>(bytes));
}

std::pair<GURL, std::u16string> Normalized(NSURL* url, NSString* title) {
  const GURL gurl =
      url ? GURL(base::SysNSStringToUTF8(url.absoluteString)) : GURL();
  return NormalizeURLForSavedTabGroup(gurl, base::SysNSStringToUTF16(title));
}

std::optional<size_t> Position(NSInteger index) {
  if (index < 0) {
    return std::nullopt;
  }
  return static_cast<size_t>(index);
}

// Queues local tab group edits until the sync service has initialized, then
// adds any saved group the service still does not know.
//
// `AddGroup` goes straight to the service, which keeps it until init.
// `AddTab`, `RemoveTab`, `MoveTab`, `NavigateTab`, `UpdateVisualData`, and
// `RemoveGroup` do nothing when the service does not know the group, so they
// wait here. `OnInitialized` runs those first. An `AddTab` for a group that
// is still unknown does nothing; the `AddGroup` that follows includes the
// group's current tabs.
//
// Pushes are dropped while the model says local observation is paused.
class TabGroupSyncBridge : public tab_groups::TabGroupSyncService::Observer {
 public:
  TabGroupSyncBridge(tab_groups::TabGroupSyncService* service,
                     id<BraveTabGroupSyncLocalModel> model)
      : service_(service), model_(model), observation_(this) {
    observation_.Observe(service);
  }

  TabGroupSyncBridge(const TabGroupSyncBridge&) = delete;
  TabGroupSyncBridge& operator=(const TabGroupSyncBridge&) = delete;
  ~TabGroupSyncBridge() override = default;

  void AddGroup(NSUUID* group_id,
                NSString* title,
                int16_t color,
                NSArray<BraveTabGroupSyncTab*>* tabs) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || !AcceptsPush()) {
      return;
    }

    const base::Uuid saved_guid = base::Uuid::GenerateRandomV4();
    std::vector<tab_groups::SavedTabGroupTab> saved_tabs;
    for (BraveTabGroupSyncTab* tab in tabs) {
      if (tab.syncTabID <= 0) {
        continue;
      }
      const std::pair<GURL, std::u16string> saved =
          Normalized(tab.URL, tab.title);
      saved_tabs.emplace_back(saved.first, saved.second, saved_guid,
                              Position(tab.index),
                              /*saved_tab_guid=*/std::nullopt, tab.syncTabID);
    }
    if (saved_tabs.empty()) {
      return;
    }

    service_->AddGroup(tab_groups::SavedTabGroup(
        base::SysNSStringToUTF16(title), ColorFromModel(color), saved_tabs,
        /*position=*/std::nullopt, saved_guid, *local_id));
  }

  void UpdateVisualData(NSUUID* group_id,
                        NSString* title,
                        int16_t color,
                        bool collapsed) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || !AcceptsPush()) {
      return;
    }
    Enqueue(base::BindOnce(
        [](tab_groups::TabGroupSyncService* service,
           tab_groups::LocalTabGroupID group,
           tab_groups::TabGroupVisualData visual) {
          service->UpdateVisualData(group, &visual);
        },
        service_.get(), *local_id,
        tab_groups::TabGroupVisualData(base::SysNSStringToUTF16(title),
                                       ColorFromModel(color), collapsed)));
  }

  void AddTab(NSUUID* group_id,
              int32_t sync_tab_id,
              NSString* title,
              NSURL* url,
              NSInteger index) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || sync_tab_id <= 0 || !AcceptsPush()) {
      return;
    }
    const std::pair<GURL, std::u16string> saved = Normalized(url, title);
    Enqueue(base::BindOnce(
        [](tab_groups::TabGroupSyncService* service,
           tab_groups::LocalTabGroupID group, tab_groups::LocalTabID tab_id,
           std::u16string saved_title, GURL saved_url,
           std::optional<size_t> position) {
          service->AddTab(group, tab_id, saved_title, saved_url, position);
        },
        service_.get(), *local_id, sync_tab_id, saved.second, saved.first,
        Position(index)));
  }

  void RemoveTab(NSUUID* group_id, int32_t sync_tab_id) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || sync_tab_id <= 0 || !AcceptsPush()) {
      return;
    }
    Enqueue(base::BindOnce(
        [](tab_groups::TabGroupSyncService* service,
           tab_groups::LocalTabGroupID group, tab_groups::LocalTabID tab_id) {
          service->RemoveTab(group, tab_id);
        },
        service_.get(), *local_id, sync_tab_id));
  }

  void MoveTab(NSUUID* group_id, int32_t sync_tab_id, NSInteger index) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || sync_tab_id <= 0 || index < 0 || !AcceptsPush()) {
      return;
    }
    Enqueue(base::BindOnce(
        [](tab_groups::TabGroupSyncService* service,
           tab_groups::LocalTabGroupID group, tab_groups::LocalTabID tab_id,
           int new_index) { service->MoveTab(group, tab_id, new_index); },
        service_.get(), *local_id, sync_tab_id, static_cast<int>(index)));
  }

  void NavigateTab(NSUUID* group_id,
                   int32_t sync_tab_id,
                   NSString* title,
                   NSURL* url) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || sync_tab_id <= 0 || !AcceptsPush()) {
      return;
    }
    const std::pair<GURL, std::u16string> saved = Normalized(url, title);
    Enqueue(base::BindOnce(
        [](tab_groups::TabGroupSyncService* service,
           tab_groups::LocalTabGroupID group, tab_groups::LocalTabID tab_id,
           GURL saved_url, std::u16string saved_title) {
          service->NavigateTab(group, tab_id, saved_url, saved_title);
        },
        service_.get(), *local_id, sync_tab_id, saved.first, saved.second));
  }

  void RemoveGroup(NSUUID* group_id) {
    const std::optional<tab_groups::LocalTabGroupID> local_id =
        LocalId(group_id);
    if (!local_id || !AcceptsPush()) {
      return;
    }
    Enqueue(base::BindOnce(
        [](tab_groups::TabGroupSyncService* service,
           tab_groups::LocalTabGroupID group) { service->RemoveGroup(group); },
        service_.get(), *local_id));
  }

  void OnInitialized() override {
    initialized_ = true;
    std::vector<base::OnceClosure> pending;
    pending.swap(pending_);
    for (base::OnceClosure& action : pending) {
      std::move(action).Run();
    }
    Reconcile();
  }

  void OnWillBeDestroyed() override {
    pending_.clear();
    initialized_ = false;
    service_ = nullptr;
    observation_.Reset();
  }

 private:
  bool AcceptsPush() const {
    return service_ && !(model_ && [model_ isLocalObservationPaused]);
  }

  void Enqueue(base::OnceClosure action) {
    if (!initialized_) {
      pending_.push_back(std::move(action));
      return;
    }
    std::move(action).Run();
  }

  void Reconcile() {
    if (!service_) {
      return;
    }
    for (NSUUID* uuid in [model_ localGroupIDs]) {
      const std::optional<tab_groups::LocalTabGroupID> local_id = LocalId(uuid);
      if (!local_id || service_->GetGroup(*local_id).has_value()) {
        continue;
      }
      std::unique_ptr<tab_groups::SavedTabGroup> saved =
          SavedTabGroupFromLocalModel(model_, *local_id);
      if (!saved) {
        continue;
      }
      service_->AddGroup(std::move(*saved));
    }
  }

  raw_ptr<tab_groups::TabGroupSyncService> service_;
  id<BraveTabGroupSyncLocalModel> model_;
  base::ScopedObservation<tab_groups::TabGroupSyncService,
                          tab_groups::TabGroupSyncService::Observer>
      observation_;
  bool initialized_ = false;
  std::vector<base::OnceClosure> pending_;
};

std::unique_ptr<TabGroupSyncBridge>& BridgeHolder() {
  static base::NoDestructor<std::unique_ptr<TabGroupSyncBridge>> holder;
  return *holder;
}

TabGroupSyncBridge* CurrentBridge() {
  return BridgeHolder().get();
}

}  // namespace

void InstallTabGroupSyncBridge(tab_groups::TabGroupSyncService* service,
                               id<BraveTabGroupSyncLocalModel> model) {
  BridgeHolder() = std::make_unique<TabGroupSyncBridge>(service, model);
}

static void PushAddGroup(NSUUID* group_id,
                         NSString* title,
                         int16_t color,
                         NSArray<BraveTabGroupSyncTab*>* tabs) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->AddGroup(group_id, title, color, tabs);
  }
}

static void PushUpdateVisualData(NSUUID* group_id,
                                 NSString* title,
                                 int16_t color,
                                 bool collapsed) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->UpdateVisualData(group_id, title, color, collapsed);
  }
}

static void PushAddTab(NSUUID* group_id,
                       int32_t sync_tab_id,
                       NSString* title,
                       NSURL* url,
                       NSInteger index) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->AddTab(group_id, sync_tab_id, title, url, index);
  }
}

static void PushRemoveTab(NSUUID* group_id, int32_t sync_tab_id) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->RemoveTab(group_id, sync_tab_id);
  }
}

static void PushMoveTab(NSUUID* group_id,
                        int32_t sync_tab_id,
                        NSInteger index) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->MoveTab(group_id, sync_tab_id, index);
  }
}

static void PushNavigateTab(NSUUID* group_id,
                            int32_t sync_tab_id,
                            NSString* title,
                            NSURL* url) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->NavigateTab(group_id, sync_tab_id, title, url);
  }
}

static void PushRemoveGroup(NSUUID* group_id) {
  if (TabGroupSyncBridge* bridge = CurrentBridge()) {
    bridge->RemoveGroup(group_id);
  }
}

}  // namespace brave

@implementation BraveTabGroupSyncTab

- (instancetype)initWithSyncTabID:(int32_t)syncTabID
                            title:(NSString*)title
                              URL:(NSURL*)URL
                            index:(NSInteger)index {
  if ((self = [super init])) {
    _syncTabID = syncTabID;
    _title = [title copy];
    _URL = [URL copy];
    _index = index;
  }
  return self;
}

@end

@implementation BraveTabGroupSyncPush

+ (void)addGroupWithID:(NSUUID*)groupID
                 title:(NSString*)title
                 color:(int16_t)color
                  tabs:(NSArray<BraveTabGroupSyncTab*>*)tabs {
  brave::PushAddGroup(groupID, title, color, tabs);
}

+ (void)updateVisualDataForGroupID:(NSUUID*)groupID
                             title:(NSString*)title
                             color:(int16_t)color
                         collapsed:(BOOL)collapsed {
  brave::PushUpdateVisualData(groupID, title, color, collapsed);
}

+ (void)addTabWithSyncTabID:(int32_t)syncTabID
                    groupID:(NSUUID*)groupID
                      title:(NSString*)title
                        URL:(NSURL*)URL
                      index:(NSInteger)index {
  brave::PushAddTab(groupID, syncTabID, title, URL, index);
}

+ (void)removeTabWithSyncTabID:(int32_t)syncTabID groupID:(NSUUID*)groupID {
  brave::PushRemoveTab(groupID, syncTabID);
}

+ (void)moveTabWithSyncTabID:(int32_t)syncTabID
                     groupID:(NSUUID*)groupID
                     toIndex:(NSInteger)index {
  brave::PushMoveTab(groupID, syncTabID, index);
}

+ (void)navigateTabWithSyncTabID:(int32_t)syncTabID
                         groupID:(NSUUID*)groupID
                           title:(NSString*)title
                             URL:(NSURL*)URL {
  brave::PushNavigateTab(groupID, syncTabID, title, URL);
}

+ (void)removeGroupWithID:(NSUUID*)groupID {
  brave::PushRemoveGroup(groupID);
}

@end

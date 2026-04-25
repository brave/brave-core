// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_service.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/to_address.h"
#include "brave/components/containers/core/browser/containers_service_observer.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/browser/unknown_container.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

ContainersService::ContainersService(PrefService* prefs,
                                     std::unique_ptr<Delegate> delegate)
    : prefs_(CHECK_DEREF(prefs)), delegate_(std::move(delegate)) {
  CHECK(delegate_);

  pref_change_registrar_.Init(base::to_address(prefs_));
  pref_change_registrar_.Add(
      prefs::kContainersList,
      base::BindRepeating(&ContainersService::OnSyncedContainersChanged,
                          base::Unretained(this)));

  ScheduleOrphanedContainersCleanup();
}

ContainersService::~ContainersService() = default;

void ContainersService::Shutdown() {
  weak_factory_.InvalidateWeakPtrs();
  pref_change_registrar_.RemoveAll();
  delegate_.reset();
}

void ContainersService::AddObserver(ContainersServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void ContainersService::RemoveObserver(ContainersServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void ContainersService::MarkContainerUsed(std::string_view container_id) {
  CHECK(!container_id.empty());

  if (HasLocallyUsedContainerInPrefs(*prefs_, container_id)) {
    return;
  }

  auto container = GetContainerFromPrefs(*prefs_, container_id);
  if (!container) {
    container = CreateUnknownContainer(container_id);
  }

  SetLocallyUsedContainerToPrefs(container, *prefs_);
}

mojom::ContainerPtr ContainersService::GetRuntimeContainerById(
    std::string_view id) const {
  if (auto container = GetContainerFromPrefs(*prefs_, id)) {
    return container;
  }
  if (auto container = GetLocallyUsedContainerFromPrefs(*prefs_, id)) {
    return container;
  }
  return nullptr;
}

std::vector<mojom::ContainerPtr> ContainersService::GetContainers() const {
  return GetContainersFromPrefs(*prefs_);
}

void ContainersService::ScheduleOrphanedContainersCleanupForTesting() {
  ScheduleOrphanedContainersCleanup();
}

void ContainersService::OnSyncedContainersChanged() {
  RefreshLocallyUsedContainersFromSyncedList();
  observers_.Notify(&ContainersServiceObserver::OnContainersListChanged);
}

void ContainersService::RefreshLocallyUsedContainersFromSyncedList() {
  for (const auto& used_container :
       GetLocallyUsedContainersFromPrefs(*prefs_)) {
    if (auto container = GetContainerFromPrefs(*prefs_, used_container->id)) {
      SetLocallyUsedContainerToPrefs(container, *prefs_);
    } else {
      // A container may be absent from the synced list if it was deleted. We
      // don't remove the used-container snapshot in this case here. It will be
      // removed with a separate cleanup logic later.
    }
  }
}

void ContainersService::ScheduleOrphanedContainersCleanup() {
  bool should_cleanup = false;

  // Check if any locally used container is not referenced by the synced
  // containers list. If so, schedule the cleanup.
  const auto& synced_containers = GetContainersFromPrefs(*prefs_);
  for (const auto& locally_used_container :
       GetLocallyUsedContainersFromPrefs(*prefs_)) {
    const std::string& id = locally_used_container->id;
    if (std::ranges::any_of(synced_containers,
                            [&](const auto& c) { return c->id == id; })) {
      continue;
    }

    should_cleanup = true;
    break;
  }

  // If any locally used container is not referenced by the synced containers
  // list, schedule the cleanup.
  if (should_cleanup) {
    delegate_->GetReferencedContainerIds(
        base::BindOnce(&ContainersService::OnReferencedContainerIdsReady,
                       weak_factory_.GetWeakPtr()));
  }
}

void ContainersService::OnReferencedContainerIdsReady(
    const base::flat_set<std::string>& referenced_container_ids) {
  const auto& synced_containers = GetContainersFromPrefs(*prefs_);
  for (const auto& locally_used_container :
       GetLocallyUsedContainersFromPrefs(*prefs_)) {
    const std::string& id = locally_used_container->id;
    if (std::ranges::any_of(synced_containers,
                            [&](const auto& c) { return c->id == id; }) ||
        referenced_container_ids.contains(id)) {
      continue;
    }

    delegate_->DeleteContainerStorage(
        id, base::BindOnce(&ContainersService::OnContainerStorageDeleted,
                           weak_factory_.GetWeakPtr(), id));
  }
}

void ContainersService::OnContainerStorageDeleted(const std::string& id,
                                                  bool success) {
  if (!success) {
    LOG(WARNING) << "Failed to delete container storage for " << id
                 << " will retry on next launch";
    return;
  }

  // If the container did not reappear in the synced containers list, remove the
  // used-container snapshot from the prefs to forget about it forever.
  if (!GetContainerFromPrefs(*prefs_, id)) {
    RemoveLocallyUsedContainerFromPrefs(id, *prefs_);
  }
}

}  // namespace containers

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/containers/core/browser/containers_service_observer.h"
#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace containers {

// Handles container-related operations.
class ContainersService : public KeyedService {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    using OnReferencedContainerIdsReadyCallback =
        base::OnceCallback<void(const base::flat_set<std::string>&)>;
    using DeleteContainerStorageCallback =
        base::OnceCallback<void(bool success)>;

    // Returns the container ids referenced by tab restore, session service and
    // currently opened tabs.
    virtual void GetReferencedContainerIds(
        OnReferencedContainerIdsReadyCallback callback) = 0;

    // Deletes the storage for the container with the given id.
    virtual void DeleteContainerStorage(
        const std::string& id,
        DeleteContainerStorageCallback callback) = 0;
  };

  ContainersService(PrefService* prefs, std::unique_ptr<Delegate> delegate);
  ~ContainersService() override;

  ContainersService(const ContainersService&) = delete;
  ContainersService& operator=(const ContainersService&) = delete;

  void Shutdown() override;

  void AddObserver(ContainersServiceObserver* observer);
  void RemoveObserver(ContainersServiceObserver* observer);

  // Caches a used-container snapshot: synced metadata when the id exists in
  // the synced list, otherwise a placeholder from `CreateUnknownContainer`.
  void MarkContainerUsed(std::string_view container_id);

  // Returns the runtime container with the given `id`. Runtime containers are
  // containers that are currently in use by the user. This can be a synced
  // container or a removed, but still used container.
  mojom::ContainerPtr GetRuntimeContainerById(std::string_view id) const;

  // Returns the list of user-editable containers.
  std::vector<mojom::ContainerPtr> GetContainers() const;

  void ScheduleOrphanedContainersCleanupForTesting();

 private:
  // Called when the synced containers list changes.
  void OnSyncedContainersChanged();

  // Refreshes used-container snapshots from the synced containers list so they
  // do not stay stale (names, icons, etc.). This is called when the synced
  // containers list changes.
  void RefreshLocallyUsedContainersFromSyncedList();

  // Schedules the cleanup of orphaned containers. Orphaned containers are
  // containers that are not referenced by any tab restore or session service.
  void ScheduleOrphanedContainersCleanup();

  // Called when the container ids referenced by the current profile are ready.
  void OnReferencedContainerIdsReady(const base::flat_set<std::string>& ids);

  // Called when the storage for the container with the given id is deleted.
  void OnContainerStorageDeleted(const std::string& id, bool success);

  raw_ref<PrefService> prefs_;
  std::unique_ptr<Delegate> delegate_;
  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<ContainersServiceObserver> observers_;
  base::WeakPtrFactory<ContainersService> weak_factory_{this};
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_

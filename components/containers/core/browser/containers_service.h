// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_

#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace containers {

// Handles container-related operations.
class ContainersService : public KeyedService {
 public:
  explicit ContainersService(PrefService* prefs);
  ~ContainersService() override;

  ContainersService(const ContainersService&) = delete;
  ContainersService& operator=(const ContainersService&) = delete;

  void Shutdown() override;

  // Caches a used-container snapshot: synced metadata when the id exists in
  // the synced list, otherwise a placeholder from `CreateUnknownContainer`.
  void MarkContainerUsed(std::string_view container_id);

  // Returns the runtime container with the given `id`. Runtime containers are
  // containers that are currently in use by the user. This can be a synced
  // container or a removed, but still used container.
  mojom::ContainerPtr GetRuntimeContainerById(std::string_view id) const;

  // Returns the list of user-editable containers.
  std::vector<mojom::ContainerPtr> GetContainers() const;

 private:
  void OnSyncedContainersChanged();

  raw_ref<PrefService> prefs_;
  PrefChangeRegistrar pref_change_registrar_;
  base::WeakPtrFactory<ContainersService> weak_factory_{this};
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/containers/core/mojom/containers.mojom-forward.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace containers {

// Handles container-related operations.
//
// TODO(https://github.com/brave/brave-browser/issues/53604): Add a local cache
// for recently used containers.
class ContainersService : public KeyedService {
 public:
  explicit ContainersService(PrefService* prefs);
  ~ContainersService() override;

  ContainersService(const ContainersService&) = delete;
  ContainersService& operator=(const ContainersService&) = delete;

  // Returns the runtime container with the given `id`.
  //
  // TODO(https://github.com/brave/brave-browser/issues/53604): Will fallback to
  // local used-containers cache.
  mojom::ContainerPtr GetRuntimeContainerById(std::string_view id) const;

  // Returns the list of user-editable containers.
  std::vector<mojom::ContainerPtr> GetContainers() const;

 private:
  raw_ref<PrefService> prefs_;
  base::WeakPtrFactory<ContainersService> weak_factory_{this};
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_CONTAINERS_SERVICE_H_

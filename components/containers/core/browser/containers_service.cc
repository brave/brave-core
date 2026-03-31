// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_service.h"

#include "base/check_deref.h"
#include "base/types/to_address.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/browser/unknown_container.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

ContainersService::ContainersService(PrefService* prefs)
    : prefs_(CHECK_DEREF(prefs)) {
  pref_change_registrar_.Init(base::to_address(prefs_));
  pref_change_registrar_.Add(
      prefs::kContainersList,
      base::BindRepeating(&ContainersService::OnSyncedContainersChanged,
                          base::Unretained(this)));
}

ContainersService::~ContainersService() = default;

void ContainersService::Shutdown() {
  pref_change_registrar_.RemoveAll();
}

void ContainersService::MarkContainerUsed(std::string_view container_id) {
  CHECK(!container_id.empty());

  if (HasUsedContainerInPrefs(*prefs_, container_id)) {
    return;
  }

  auto container = GetContainerFromPrefs(*prefs_, container_id);
  if (!container) {
    container = CreateUnknownContainer(container_id);
  }

  SetUsedContainerToPrefs(container, *prefs_);
}

mojom::ContainerPtr ContainersService::GetRuntimeContainerById(
    std::string_view id) const {
  if (auto container = GetContainerFromPrefs(*prefs_, id)) {
    return container;
  }
  if (auto container = GetUsedContainerFromPrefs(*prefs_, id)) {
    return container;
  }
  return nullptr;
}

std::vector<mojom::ContainerPtr> ContainersService::GetContainers() const {
  return GetContainersFromPrefs(*prefs_);
}

void ContainersService::OnSyncedContainersChanged() {
  RefreshUsedContainersFromSyncedList();
}

void ContainersService::RefreshUsedContainersFromSyncedList() {
  for (const auto& used_container : GetUsedContainersFromPrefs(*prefs_)) {
    if (auto container = GetContainerFromPrefs(*prefs_, used_container->id)) {
      SetUsedContainerToPrefs(container, *prefs_);
    } else {
      // A container may be absent from the synced list if it was deleted. We
      // don't remove the used-container snapshot in this case here. It will be
      // removed with a separate cleanup logic later.
    }
  }
}

}  // namespace containers

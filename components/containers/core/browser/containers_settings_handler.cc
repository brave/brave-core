// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_settings_handler.h"

#include <utility>

#include "base/uuid.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs.h"

namespace containers {

ContainersSettingsHandler::ContainersSettingsHandler(
    mojo::PendingRemote<mojom::ContainersSettingsObserver> observer,
    PrefService* prefs,
    std::unique_ptr<Delegate> delegate)
    : observer_(std::move(observer)),
      prefs_(prefs),
      delegate_(std::move(delegate)) {
  DCHECK(prefs_);
  DCHECK(delegate_);
  pref_change_registrar_.Init(prefs);
  // Watch for external changes to containers list (e.g. sync, other windows)
  pref_change_registrar_.Add(
      prefs::kContainersList,
      base::BindRepeating(&ContainersSettingsHandler::OnContainersChanged,
                          base::Unretained(this)));
}

ContainersSettingsHandler::~ContainersSettingsHandler() {}

void ContainersSettingsHandler::GetContainers(GetContainersCallback callback) {
  std::move(callback).Run(GetContainersFromPrefs(*prefs_));
}

void ContainersSettingsHandler::AddOrUpdateContainer(
    mojom::ContainerPtr container) {
  CHECK(!container->name.empty());

  auto containers = GetContainersFromPrefs(*prefs_);

  if (container->id.empty()) {
    // Create a new container if it doesn't have an ID.
    container->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
    containers.push_back(std::move(container));
  } else {
    // Update an existing container.
    for (auto& c : containers) {
      if (c->id == container->id) {
        c = std::move(container);
        break;
      }
      // If the container is not found, it means it was deleted. Update should
      // never add a new container.
    }
  }

  SetContainersToPrefs(std::move(containers), *prefs_);
}

void ContainersSettingsHandler::RemoveContainer(
    const std::string& id,
    RemoveContainerCallback callback) {
  // First remove all container data (cookies, storage etc.) via the delegate.
  delegate_->RemoveContainerData(
      id,
      base::BindOnce(&ContainersSettingsHandler::OnContainerDataRemoved,
                     weak_ptr_factory_.GetWeakPtr(), id, std::move(callback)));
}

void ContainersSettingsHandler::OnContainerDataRemoved(
    const std::string& id,
    RemoveContainerCallback callback) {
  // Update container list only after data cleanup is complete.
  auto containers = GetContainersFromPrefs(*prefs_);
  std::erase_if(containers, [id](const auto& c) { return c->id == id; });
  SetContainersToPrefs(std::move(containers), *prefs_);
  // TODO(https://github.com/brave/brave-browser/issues/46352): Replace with a
  // sync call. For now we simulate async data removal.
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, std::move(callback), base::Seconds(5));
}

void ContainersSettingsHandler::OnContainersChanged() {
  // Notify observer about container list changes (from this window or others).
  observer_->OnContainersChanged(GetContainersFromPrefs(*prefs_));
}

}  // namespace containers

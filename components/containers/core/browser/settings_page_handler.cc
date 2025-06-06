// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/settings_page_handler.h"

#include <utility>

#include "base/uuid.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/browser/prefs.h"

namespace containers {

SettingsPageHandler::SettingsPageHandler(
    mojo::PendingRemote<mojom::SettingsPage> page,
    PrefService* prefs,
    std::unique_ptr<Delegate> delegate)
    : page_(std::move(page)), prefs_(prefs), delegate_(std::move(delegate)) {
  DCHECK(prefs_);
  DCHECK(delegate_);
  pref_change_registrar_.Init(prefs);
  // Watch for external changes to containers list (e.g. sync, other windows)
  pref_change_registrar_.Add(
      prefs::kContainersList,
      base::BindRepeating(&SettingsPageHandler::OnContainersChanged,
                          base::Unretained(this)));
}

SettingsPageHandler::~SettingsPageHandler() {}

void SettingsPageHandler::GetContainers(GetContainersCallback callback) {
  std::move(callback).Run(GetContainerList(*prefs_));
}

void SettingsPageHandler::AddOrUpdateContainer(mojom::ContainerPtr container) {
  CHECK(!container->name.empty());

  auto containers = GetContainerList(*prefs_);

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
    }
  }

  SetContainerList(std::move(containers), *prefs_);
}

void SettingsPageHandler::RemoveContainer(const std::string& id,
                                          RemoveContainerCallback callback) {
  // First remove all container data (cookies, storage etc.) via the delegate.
  delegate_->RemoveContainerData(
      id,
      base::BindOnce(&SettingsPageHandler::OnContainerDataRemoved,
                     weak_ptr_factory_.GetWeakPtr(), id, std::move(callback)));
}

void SettingsPageHandler::OnContainerDataRemoved(
    const std::string& id,
    RemoveContainerCallback callback) {
  // Update container list only after data cleanup is complete.
  auto containers = GetContainerList(*prefs_);
  std::erase_if(containers, [id](const auto& c) { return c->id == id; });
  SetContainerList(std::move(containers), *prefs_);
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, std::move(callback), base::Seconds(5));
}

void SettingsPageHandler::OnContainersChanged() {
  // Notify WebUI about container list changes (from this window or others).
  page_->OnContainersChanged(GetContainerList(*prefs_));
}

}  // namespace containers

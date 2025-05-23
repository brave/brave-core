// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/settings_page_handler.h"

#include <utility>

#include "base/uuid.h"
#include "brave/components/containers/core/browser/prefs.h"

namespace containers {

SettingsPageHandler::SettingsPageHandler(
    mojo::PendingRemote<mojom::SettingsPage> page,
    PrefService* prefs)
    : page_(std::move(page)), prefs_(prefs) {
  DCHECK(prefs_);
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      prefs::kContainersList,
      base::BindRepeating(&SettingsPageHandler::OnContainersChanged,
                          base::Unretained(this)));
}

SettingsPageHandler::~SettingsPageHandler() {}

void SettingsPageHandler::GetContainers(GetContainersCallback callback) {
  std::move(callback).Run(GetContainersList(*prefs_));
}

void SettingsPageHandler::AddContainer(mojom::ContainerPtr container) {
  CHECK(container->id.empty());
  CHECK(!container->name.empty());
  container->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  auto containers = GetContainersList(*prefs_);
  containers.push_back(std::move(container));
  SetContainersList(*prefs_, std::move(containers));
}

void SettingsPageHandler::UpdateContainer(mojom::ContainerPtr container) {
  CHECK(!container->id.empty());
  CHECK(!container->name.empty());
  auto containers = GetContainersList(*prefs_);
  for (auto& c : containers) {
    if (c->id == container->id) {
      c = std::move(container);
      break;
    }
  }
  SetContainersList(*prefs_, std::move(containers));
}

void SettingsPageHandler::RemoveContainer(const std::string& id) {
  auto containers = GetContainersList(*prefs_);
  std::erase_if(containers, [id](const auto& c) { return c->id == id; });
  SetContainersList(*prefs_, std::move(containers));
}

void SettingsPageHandler::OnContainersChanged() {
  page_->OnContainersChanged(GetContainersList(*prefs_));
}

}  // namespace containers

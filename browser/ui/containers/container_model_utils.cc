// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/container_model_utils.h"

#include "base/notimplemented.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/components/containers/core/browser/prefs.h"

namespace containers {

std::vector<ContainerModel> GetContainerModelsFromPrefs(
    const PrefService& prefs) {
  std::vector<ContainerModel> containers;
  for (auto& container : containers::GetContainersFromPrefs(prefs)) {
    containers.emplace_back(std::move(container),
                            GetImageModelForContainer(container));
  }
  return containers;
}

ui::ImageModel GetImageModelForContainer(
    const containers::mojom::ContainerPtr& container) {
  NOTIMPLEMENTED();
  return ui::ImageModel();
}

}  // namespace containers

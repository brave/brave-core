// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/container_model_utils.h"

#include <utility>

#include "base/notimplemented.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/components/containers/core/browser/prefs.h"

namespace containers {

std::vector<ContainerModel> GetContainerModelsFromPrefs(
    const PrefService& prefs) {
  std::vector<ContainerModel> containers;
  for (auto& container : GetContainersFromPrefs(prefs)) {
    containers.emplace_back(std::move(container));
  }
  return containers;
}

ui::ImageModel GetImageModelForContainer(const mojom::ContainerPtr& container) {
  CHECK(container) << "Container must be valid";
  // TODO(https://github.com/brave/brave-browser/issues/47117)
  // At the moment, we don't have data filled into `container` to decide which
  // icon to use to represent the container. When the data is ready, implement
  // this function to return the appropriate icon based on the container data.
  NOTIMPLEMENTED();
  return ui::ImageModel();
}

}  // namespace containers

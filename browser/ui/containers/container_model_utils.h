// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINER_MODEL_UTILS_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINER_MODEL_UTILS_H_

#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "ui/base/models/image_model.h"

class PrefService;

namespace containers {

class ContainerModel;

// Returns a vector of `ContainerModel` objects created from preferences. This
// function retrieves the containers defined in the preferences and converts
// them into `ContainerModel` instances, which also contains UI specific data
// such as icons.
std::vector<ContainerModel> GetContainerModelsFromPrefs(
    const PrefService& prefs);

// Returns an image model for the given container. This function is useful for
// testability.
ui::ImageModel GetImageModelForContainer(
    const containers::mojom::ContainerPtr& container);

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINER_MODEL_UTILS_H_

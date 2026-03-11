// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/container_model.h"

#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/browser/ui/containers/containers_icon_generator.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/unknown_container.h"
#include "ui/gfx/geometry/size.h"

namespace containers {

namespace {

ui::ImageModel GetImageModelForContainer(const mojom::ContainerPtr& container,
                                         float scale_factor) {
  CHECK(container) << "Container must be valid";
  constexpr int kDipSize = 16;
  constexpr int kDipIconSize = 12;
  return ui::ImageModel::FromImageGenerator(
      base::BindRepeating(&GenerateContainerIcon, container->icon,
                          container->background_color, kDipSize, kDipIconSize,
                          scale_factor),
      gfx::Size(kDipSize, kDipSize));
}

}  // namespace

ContainerModel::ContainerModel(mojom::ContainerPtr container,
                               float scale_factor)
    : container_(std::move(container)),
      icon_(GetImageModelForContainer(container_, scale_factor)) {
  CHECK(container_) << "Container must be valid";
}

ContainerModel::ContainerModel(ContainerModel&& other) noexcept = default;

ContainerModel& ContainerModel::operator=(ContainerModel&& other) noexcept =
    default;

ContainerModel::~ContainerModel() = default;

std::vector<ContainerModel> GetContainerModels(const ContainersService& service,
                                               float scale_factor) {
  std::vector<ContainerModel> containers;
  for (auto& container : service.GetContainers()) {
    containers.emplace_back(std::move(container), scale_factor);
  }
  return containers;
}

ContainerModel GetRuntimeContainerModel(const ContainersService& service,
                                        std::string_view id,
                                        float scale_factor) {
  if (auto container = service.GetRuntimeContainerById(id)) {
    return ContainerModel(std::move(container), scale_factor);
  }

  return ContainerModel(CreateUnknownContainer(std::string(id)), scale_factor);
}

}  // namespace containers

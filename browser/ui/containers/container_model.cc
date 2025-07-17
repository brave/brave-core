// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/container_model.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/containers/containers_icon_generator.h"
#include "ui/gfx/geometry/size.h"

namespace containers {

namespace {

ui::ImageModel GetImageModelForContainer(const mojom::ContainerPtr& container,
                                         float scale_factor) {
  CHECK(container) << "Container must be valid";
  constexpr int kDipSize = 16;
  return ui::ImageModel::FromImageGenerator(
      base::BindRepeating(&GenerateContainerIcon, container->icon,
                          container->background_color, kDipSize, scale_factor),
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

}  // namespace containers

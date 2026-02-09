// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINER_MODEL_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINER_MODEL_H_

#include <string>
#include <vector>

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "ui/base/models/image_model.h"

class PrefService;

namespace containers {

// A model for view that represents a container in the UI.
class ContainerModel {
 public:
  // As containers list is syncable, there could be a case where the container
  // is not found from preferences but it is still needed to be represented in
  // the UI. In this case, we use this method to create a ContainerModel for
  // the unknown container with default UI representation.
  static ContainerModel CreateForUnknown(const std::string& id,
                                         float scale_factor);

  ContainerModel(mojom::ContainerPtr container, float scale_factor);
  ContainerModel(const ContainerModel&) = delete;
  ContainerModel& operator=(const ContainerModel&) = delete;
  ContainerModel(ContainerModel&& other) noexcept;
  ContainerModel& operator=(ContainerModel&& other) noexcept;
  ~ContainerModel();

  const std::string& id() const { return container_->id; }
  const std::string& name() const { return container_->name; }
  const ui::ImageModel& icon() const { return icon_; }
  const SkColor& background_color() const {
    return container_->background_color;
  }
  const mojom::ContainerPtr& container() const { return container_; }

 private:
  mojom::ContainerPtr container_;

  // An icon representing the `container_`. Each container can have its icon.
  // This icon can be used by UI, such as menus and tabs.
  ui::ImageModel icon_;
};

// Builds ContainerModels from prefs; shared by menu model, tab strip, etc.
std::vector<ContainerModel> GetContainerModelsFromPrefs(
    const PrefService& prefs,
    float scale_factor);

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINER_MODEL_H_

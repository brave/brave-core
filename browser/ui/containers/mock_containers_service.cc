// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/mock_containers_service.h"

// static
MockContainersService& MockContainersService::GetInstance() {
  static MockContainersService instance;
  return instance;
}

MockContainersService::MockContainersService() = default;

MockContainersService::~MockContainersService() = default;

void MockContainersService::AddContainer(const ContainerItem& item) {
  containers_.push_back(item);
}

void MockContainersService::RemoveContainerById(int id) {
  containers_.erase(
      std::remove_if(containers_.begin(), containers_.end(),
                     [id](const ContainerItem& item) { return item.id == id; }),
      containers_.end());
}

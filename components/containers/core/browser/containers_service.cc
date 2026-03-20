// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_service.h"

#include "base/check_deref.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/prefs/pref_service.h"

namespace containers {

ContainersService::ContainersService(PrefService* prefs)
    : prefs_(CHECK_DEREF(prefs)) {}

ContainersService::~ContainersService() = default;

mojom::ContainerPtr ContainersService::GetRuntimeContainerById(
    std::string_view id) const {
  if (auto container = GetContainerFromPrefs(*prefs_, id)) {
    return container;
  }
  return nullptr;
}

std::vector<mojom::ContainerPtr> ContainersService::GetContainers() const {
  return GetContainersFromPrefs(*prefs_);
}

}  // namespace containers

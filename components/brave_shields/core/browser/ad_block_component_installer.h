// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <string>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "crypto/sha2.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_shields {

using OnComponentReadyCallback =
    base::RepeatingCallback<void(const base::FilePath& install_path)>;

void RegisterAdBlockDefaultResourceComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback);

void RegisterAdBlockFilterListCatalogComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback);

void RegisterAdBlockFiltersComponent(
    component_updater::ComponentUpdateService* cus,
    base::span<const uint8_t, crypto::kSHA256Length>
        component_public_key_sha256,
    const std::string& component_name,
    OnComponentReadyCallback callback);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_COMPONENT_INSTALLER_H_

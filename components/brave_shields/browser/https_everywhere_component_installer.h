/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_shields {

using OnComponentReadyCallback =
    base::RepeatingCallback<void(const base::FilePath& install_path)>;

void RegisterHTTPSEverywhereComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_

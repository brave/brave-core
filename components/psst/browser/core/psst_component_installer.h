// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_COMPONENT_INSTALLER_H_

#include "base/component_export.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace psst {

// Registers the PSST component with the component updater.
COMPONENT_EXPORT(PSST_BROWSER_CORE)
void RegisterPsstComponent(component_updater::ComponentUpdateService* cus);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_COMPONENT_INSTALLER_H_

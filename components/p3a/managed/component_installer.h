/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace p3a {

class P3AService;

// Register P3A component if P3A is enabled, unregister if disabled.
void MaybeToggleP3AComponent(component_updater::ComponentUpdateService* cus,
                             P3AService* p3a_service);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_COMPONENT_INSTALLER_H_

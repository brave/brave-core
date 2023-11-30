// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_PLAYER_BROWSER_CORE_BRAVE_BROWSER_PLAYER_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_PLAYER_BROWSER_CORE_BRAVE_BROWSER_PLAYER_COMPONENT_INSTALLER_H_

#include <string>

#include "base/component_export.h"
#include "base/functional/callback.h"

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_player {

// Registers the Brave Player component with the component updater.
// |on_ready| is called when the component is ready to be used with
// base::FilaPath that can be used to load the component. |on_registered| is
// called when the component is registered with the component id.
COMPONENT_EXPORT(BRAVE_PLAYER_CORE_BROWSER)
void RegisterBravePlayerComponent(
    component_updater::ComponentUpdateService* cus,
    base::OnceCallback<void(const base::FilePath&)> on_ready,
    base::OnceCallback<void(const std::string&)> on_registered);

}  // namespace brave_player

#endif  // BRAVE_COMPONENTS_BRAVE_PLAYER_BROWSER_CORE_BRAVE_BROWSER_PLAYER_COMPONENT_INSTALLER_H_

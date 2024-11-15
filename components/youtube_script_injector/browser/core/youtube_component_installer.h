// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_COMPONENT_INSTALLER_H_

#include "base/component_export.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace youtube_script_injector {

// Registers the Youtube Script Injector component with the component updater.
COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE)
void RegisterYouTubeComponent(component_updater::ComponentUpdateService* cus);

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_COMPONENT_INSTALLER_H_

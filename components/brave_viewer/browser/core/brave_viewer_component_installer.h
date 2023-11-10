// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CORE_BRAVE_VIEWER_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CORE_BRAVE_VIEWER_COMPONENT_INSTALLER_H_

#include <string>

#include "base/component_export.h"
#include "base/functional/callback.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_viewer {

// Registers the Brave Viewer component with the component updater.
COMPONENT_EXPORT(BRAVE_VIEWER_BROWSER_CORE)
void RegisterBraveViewerComponent(
    component_updater::ComponentUpdateService* cus,
    base::OnceCallback<void(const std::string&)> cb);

}  // namespace brave_viewer

#endif  // BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CORE_BRAVE_VIEWER_COMPONENT_INSTALLER_H_

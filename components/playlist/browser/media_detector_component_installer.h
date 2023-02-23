/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_MEDIA_DETECTOR_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_MEDIA_DETECTOR_COMPONENT_INSTALLER_H_

#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace playlist {

using OnComponentReadyCallback =
    base::RepeatingCallback<void(const base::FilePath& install_path)>;

void RegisterMediaDetectorComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_MEDIA_DETECTOR_COMPONENT_INSTALLER_H_

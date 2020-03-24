/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_COMPONENT_INSTALLER_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/regional_component_data.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace ntp_background_images {

using OnComponentReadyCallback =
      base::RepeatingCallback<void(const base::FilePath& install_path)>;

void RegisterNTPBackgroundImagesComponent(
    component_updater::ComponentUpdateService* cus,
    const RegionalComponentData& regional_component_data,
    OnComponentReadyCallback callback);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_BACKGROUND_IMAGES_COMPONENT_INSTALLER_H_

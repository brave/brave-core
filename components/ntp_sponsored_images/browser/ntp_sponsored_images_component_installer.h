/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

class NTPSponsoredImagesComponentManager;

void RegisterNTPSponsoredImagesComponent(
    component_updater::ComponentUpdateService* cus,
    NTPSponsoredImagesComponentManager* manager);

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_NTP_SPONSORED_IMAGES_COMPONENT_INSTALLER_H_

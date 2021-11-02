/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_shields {

void RegisterHTTPSEverywhereComponent(
    component_updater::ComponentUpdateService* cus);

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_

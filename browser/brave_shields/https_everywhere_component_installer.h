/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_

#include <string>

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_shields {

void SetHTTPSEverywhereComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key);

void RegisterHTTPSEverywhereComponent(
    component_updater::ComponentUpdateService* cus);

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_HTTPS_EVERYWHERE_COMPONENT_INSTALLER_H_

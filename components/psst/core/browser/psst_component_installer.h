// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace psst {

inline constexpr char kPsstComponentName[] =
    "Brave Privacy Settings Selection for Sites Tool (PSST) Files";
inline constexpr char kPsstComponentId[] = "bchfnigamfmpeanhekjggkphjfobpipo";
inline constexpr char kPsstComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAz7OnL/yxxX0a/"
    "IKLJc2LCQ5k12S6uQFuSCkruf7mGLuDhepKo8lA5orJQI8dqUMivTmxXC7SWYVS9uj05b9LTcL"
    "tmlcNiUjdYFoCzmdtRh6rBTCzTZ7wkyyhacUpY7N3BRIR5dRk1OLfx2ovm8BLQqak3YJ7dsPxD"
    "29714xPlbaMXDCsXEgibaGlXSNpDuCKFtzhVuRhSD6hRRQ7OgLQ7vm0b5BECO/"
    "jRuMHra4G4S9Z0rRN8KA9dC38t55O+FeOGZMhjJzBEJPk5AlQzlbPGq/"
    "MVPUu+4XFNEUoaeu65PjoAc05apRFCQQ/lcNy5gQfzwVExTNyrdP62eoRf+ANhQIDAQAB";

// Registers the PSST component with the component updater.
void RegisterPsstComponent(component_updater::ComponentUpdateService* cus);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_

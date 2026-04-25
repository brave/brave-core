// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace psst {

inline constexpr char kPsstComponentName[] =
    "Brave Privacy Settings Selection for Sites Tool (PSST) Files";
inline constexpr char kPsstComponentId[] = "lhhcaamjbmbijmjbnnodjaknblkiagon";
inline constexpr char kPsstComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAphUFFHyK+"
    "qUOXSw3OJXRQwKs79bt7zqnmkeFp/szXmmhj6/"
    "i4fmNiXVaxFuVOryM9OiaVxBIGHjN1BWYCQdylgbmgVTqLWpJAy/AAKEH9/"
    "Q68yWfQnN5sg1miNir+0I1SpCiT/Dx2N7s28WNnzD2e6/"
    "7Umx+zRXkRtoPX0xAecgUeyOZcrpZXJ4CG8dTJInhv7Fly/U8V/KZhm6ydKlibwsh2CB588/"
    "FlvQUzi5ZykXnPfzlsNLyyQ8fy6/+8hzSE5x4HTW5fy3TIRvmDi/"
    "7HmW+evvuMIPl1gtVe4HKOZ7G8UaznjXBfspszHU1fqTiZWeCPb53uemo1a+rdnSHXwIDAQAB";

// Registers the PSST component with the component updater.
void RegisterPsstComponent(component_updater::ComponentUpdateService* cus);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_COMPONENT_INSTALLER_H_

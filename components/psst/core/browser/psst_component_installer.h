// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <iterator>

#include "crypto/sha2.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace psst {

inline constexpr char kPsstComponentName[] =
    "Brave Privacy Settings Selection for Sites Tool (PSST) Files";
inline constexpr uint8_t kPsstComponentPublicKeySHA256[32] = {
    0x12, 0x75, 0xd8, 0x60, 0xc5, 0xcf, 0x40, 0xd7, 0x4a, 0x96, 0x6a,
    0xf7, 0x95, 0xe1, 0xf8, 0xfe, 0xc7, 0x3c, 0x95, 0x80, 0x00, 0x92,
    0x03, 0xe0, 0x99, 0x80, 0x94, 0x66, 0xfd, 0x7f, 0xb2, 0x74};
static_assert(std::size(kPsstComponentPublicKeySHA256) ==
              crypto::kSHA256Length);

// Registers the PSST component with the component updater.
void RegisterPsstComponent(component_updater::ComponentUpdateService* cus);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_

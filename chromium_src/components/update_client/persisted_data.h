/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_

#include "brave/components/widevine/static_buildflags.h"

#include "src/components/update_client/persisted_data.h"  // IWYU pragma: export

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

class PrefService;

namespace update_client {

bool UpstreamHasArm64Widevine(PrefService* prefService);

void SetUpstreamHasArm64Widevine(PrefService* prefService);

}  // namespace update_client

#endif  // BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_

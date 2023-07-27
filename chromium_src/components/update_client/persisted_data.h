/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_

#include "brave/components/widevine/static_buildflags.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define pref_service_ \
  pref_service_;      \
  friend class SequentialUpdateChecker
#endif

#include "src/components/update_client/persisted_data.h"  // IWYU pragma: export

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#undef pref_service_
#endif

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_PERSISTED_DATA_H_

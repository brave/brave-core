/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/widevine/static_buildflags.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define RegisterPersistedDataPrefs RegisterPersistedDataPrefs_ChromiumImpl
#endif

#include "src/components/update_client/persisted_data.cc"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

#undef RegisterPersistedDataPrefs

namespace update_client {

void RegisterPersistedDataPrefs(PrefRegistrySimple* registry) {
  RegisterPersistedDataPrefs_ChromiumImpl(registry);
  registry->RegisterBooleanPref(kUpstreamHasArm64WidevineKey, false);
}

}  // namespace update_client

#endif  // BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

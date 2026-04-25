/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/buildflags/buildflags.h"

namespace gcapi_internals {

#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
const wchar_t kChromeRegClientsKey[] =
    L"Software\\BraveSoftware\\Update\\Clients\\"
    L"{F1EF32DE-F987-4289-81D2-6C4780027F9B}";
const wchar_t kChromeRegClientStateKey[] =
    L"Software\\BraveSoftware\\Update\\ClientState\\"
    L"{F1EF32DE-F987-4289-81D2-6C4780027F9B}";
#else
const wchar_t kChromeRegClientsKey[] =
    L"Software\\BraveSoftware\\Update\\Clients\\"
    L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
const wchar_t kChromeRegClientStateKey[] =
    L"Software\\BraveSoftware\\Update\\ClientState\\"
    L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

}  // namespace gcapi_internals

#include <chrome/installer/gcapi/google_update_util.cc>

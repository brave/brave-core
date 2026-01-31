/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/installer/mini_installer/appid.h"

namespace google_update {

#if defined(OFFICIAL_BUILD)
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Brave Origin uses separate app GUIDs from Brave Browser to allow
// side-by-side installation and independent update infrastructure.
const wchar_t kAppGuid[] = L"{F1EF32DE-F987-4289-81D2-6C4780027F9B}";
const wchar_t kBetaAppGuid[] = L"{56DA94FD-D872-416B-BFC4-1D7011DA7473}";
const wchar_t kDevAppGuid[] = L"{716D6A4A-D071-47A8-AC64-DBDE3EE3797B}";
const wchar_t kSxSAppGuid[] = L"{50474E96-9CD2-4BC8-B0A7-0D4B6EF2E709}";
#else
const wchar_t kAppGuid[] = L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
const wchar_t kBetaAppGuid[] = L"{103BD053-949B-43A8-9120-2E424887DE11}";
const wchar_t kDevAppGuid[] = L"{CB2150F2-595F-4633-891A-E39720CE0531}";
const wchar_t kSxSAppGuid[] = L"{C6CB981E-DB30-4876-8639-109F8933582C}";
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#else
const wchar_t kAppGuid[] = L"";
#endif

}  // namespace google_update

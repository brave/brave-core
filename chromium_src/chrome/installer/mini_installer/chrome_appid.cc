/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/mini_installer/appid.h"

namespace google_update {

#if defined(OFFICIAL_BUILD)
const wchar_t kAppGuid[] = L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
const wchar_t kMultiInstallAppGuid[] =
    L"{F7526127-0B8A-406F-8998-282BEA40103A}";
const wchar_t kBetaAppGuid[] = L"{103BD053-949B-43A8-9120-2E424887DE11}";
const wchar_t kDevAppGuid[] = L"{CB2150F2-595F-4633-891A-E39720CE0531}";
const wchar_t kSxSAppGuid[] = L"{C6CB981E-DB30-4876-8639-109F8933582C}";
#else
const wchar_t kAppGuid[] = L"";
const wchar_t kMultiInstallAppGuid[] = L"";
#endif

}  // namespace google_update

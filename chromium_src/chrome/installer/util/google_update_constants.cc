/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/util/google_update_constants.h"

#define kChromeUpgradeCode kChromeUpgradeCode_Unused
#define kGoogleUpdateUpgradeCode kGoogleUpdateUpgradeCode_Unused
#define kGoogleUpdateSetupExe kGoogleUpdateSetupExe_Unused
#define kRegPathClients kRegPathClients_Unused
#define kRegPathClientState kRegPathClientState_Unused
#define kRegPathClientStateMedium kRegPathClientStateMedium_Unused
#define kRegPathGoogleUpdate kRegPathGoogleUpdate_Unused

#include "src/chrome/installer/util/google_update_constants.cc"

#undef kChromeUpgradeCode
#undef kGoogleUpdateUpgradeCode
#undef kGoogleUpdateSetupExe
#undef kRegPathClients
#undef kRegPathClientState
#undef kRegPathClientStateMedium
#undef kRegPathGoogleUpdate

namespace google_update {

const wchar_t kChromeUpgradeCode[] = L"{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
const wchar_t kGoogleUpdateUpgradeCode[] =
    L"{B131C935-9BE6-41DA-9599-1F776BEB8019}";
const wchar_t kGoogleUpdateSetupExe[] = L"BraveUpdateSetup.exe";
const wchar_t kRegPathClients[] = L"Software\\BraveSoftware\\Update\\Clients";
const wchar_t kRegPathClientState[] =
    L"Software\\BraveSoftware\\Update\\ClientState";
const wchar_t kRegPathClientStateMedium[] =
    L"Software\\BraveSoftware\\Update\\ClientStateMedium";
const wchar_t kRegPathGoogleUpdate[] = L"Software\\BraveSoftware\\Update";

}  // namespace google_update

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/buildflags/buildflags.h"

namespace component_updater {
namespace {

// Brave Update group policy settings.
const wchar_t kGoogleUpdatePoliciesKey[] =
    L"SOFTWARE\\Policies\\BraveSoftware\\Update";
const wchar_t kCheckPeriodOverrideMinutes[] = L"AutoUpdateCheckPeriodMinutes";
const wchar_t kUpdatePolicyValue[] = L"UpdateDefault";
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
const wchar_t kChromeUpdatePolicyOverride[] =
    L"Update{F1EF32DE-F987-4289-81D2-6C4780027F9B}";
#else
const wchar_t kChromeUpdatePolicyOverride[] =
    L"Update{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)

// Don't allow update periods longer than six weeks (Chrome release cadence).
const int kCheckPeriodOverrideMinutesMax = 60 * 24 * 7 * 6;

// Brave Update registry settings.
const wchar_t kRegPathGoogleUpdate[] = L"Software\\BraveSoftware\\Update";
const wchar_t kRegPathClientsGoogleUpdate[] =
    L"Software\\BraveSoftware\\Update\\Clients\\"
    L"{B131C935-9BE6-41DA-9599-1F776BEB8019}";

}  // namespace
}  // namespace component_updater

#include <chrome/browser/component_updater/updater_state_win.cc>

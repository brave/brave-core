/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

namespace update_client {

namespace {

const int kCheckPeriodOverrideMinutesMax = 60 * 24 * 7 * 6;
const wchar_t kGoogleUpdatePoliciesKey[] =
    L"SOFTWARE\\Policies\\BraveSoftware\\Update";
const wchar_t kCheckPeriodOverrideMinutes[] = L"AutoUpdateCheckPeriodMinutes";
const wchar_t kUpdatePolicyValue[] = L"UpdateDefault";
const wchar_t kChromeUpdatePolicyOverride[] =
    L"Update{AFE6A462-C574-4B8A-AF43-4CC60DF4563B}";
const wchar_t kRegPathGoogleUpdate[] = L"Software\\BraveSoftware\\Update";
const wchar_t kRegPathClientsGoogleUpdate[] =
    L"Software\\BraveSoftware\\Update\\Clients\\"
    L"{B131C935-9BE6-41DA-9599-1F776BEB8019}";

}  // namespace

}  // namespace update_client

#include "../../../../components/update_client/updater_state_win.cc"

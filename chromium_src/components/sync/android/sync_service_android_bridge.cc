/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/android/sync_service_android_bridge.h"

#include "components/sync/service/sync_user_settings.h"

#define KeepAccountSettingsPrefsOnlyForUsers \
  KeepAccountSettingsPrefsOnlyForUsers_Unused
#include "src/components/sync/android/sync_service_android_bridge.cc"
#undef KeepAccountSettingsPrefsOnlyForUsers

namespace syncer {

// Along with SyncUserSettings::KeepAccountSettingsPrefsOnlyForUsers_Unused
// makes this method do nothing.
// We need it on Android because Brave Browser doesn't use Google Account
// to run Brave Sync. Otherwise empty gaia_ids arrives to
// SyncTransportDataPrefs::KeepAccountSettingsPrefsOnlyForUsers, where
// "Clears all account-keyed preferences for all accounts that are NOT in
// `available_gaia_ids`." So all kSyncTransportDataPerAccount gets wiped.
// Then ValidateSyncTransportData at sync_engine_impl.cc fails and
// SyncEngineImpl::Initialize overwrites sync transport prefs. Device with new
// generated cache guid is sent to the chain and all other devices see the
// duplicated entry in addition to other possible mess.
// To avoid this, override with empty implementation.
void SyncServiceAndroidBridge::KeepAccountSettingsPrefsOnlyForUsers(
    JNIEnv* env,
    const base::android::JavaParamRef<jobjectArray>& gaia_ids) {}

}  // namespace syncer

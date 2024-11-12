/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/service/sync_internals_util.h"

#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "components/os_crypt/sync/os_crypt.h"

#define ConstructAboutInformation ConstructAboutInformation_ChromiumImpl
#include "src/components/sync/service/sync_internals_util.cc"
#undef ConstructAboutInformation

namespace syncer::sync_ui_util {

base::Value::Dict ConstructAboutInformation(
    IncludeSensitiveData include_sensitive_data,
    SyncService* service,
    const std::string& channel) {
  auto about_info = ConstructAboutInformation_ChromiumImpl(
      include_sensitive_data, service, channel);

  Section section_brave_sync("Brave Sync", /*is_sensitive=*/false);

  Stat<bool>* is_passphrase_set =
      section_brave_sync.AddBoolStat("Passphrase is set");
  BraveSyncServiceImpl* brave_sync_service =
      static_cast<BraveSyncServiceImpl*>(service);
  bool failed_to_decrypt = false;
  std::string seed = brave_sync_service->prefs().GetSeed(&failed_to_decrypt);
  // If the passphrase has been set, either we can see it or we failed to
  // decrypt it
  bool is_passphrase_set_val = !seed.empty() || failed_to_decrypt;
  is_passphrase_set->Set(is_passphrase_set_val);

  // OSCrypt behavior varies depending on OS. It is possible that
  // OSCrypt::IsEncryptionAvailable reports false, but OSCrypt::DecryptString
  // succeeds. So put the additional field with actual decryption result.
  if (failed_to_decrypt) {
    Stat<bool>* failed_to_decrypt_passphrase =
        section_brave_sync.AddBoolStat("Passphrase decryption failed");
    failed_to_decrypt_passphrase->Set(true);
  }

  Stat<bool>* is_os_encryption_available =
      section_brave_sync.AddBoolStat("OS encryption available");
  is_os_encryption_available->Set(OSCrypt::IsEncryptionAvailable());

  Stat<std::string>* leave_chain_details =
      section_brave_sync.AddStringStat("Leave chain details");
  leave_chain_details->Set(brave_sync_service->prefs().GetLeaveChainDetails());

  base::Value::List* details = about_info.FindList(kDetailsKey);
  DCHECK_NE(details, nullptr);

  details->Append(section_brave_sync.ToValue());
  return about_info;
}

}  // namespace syncer::sync_ui_util

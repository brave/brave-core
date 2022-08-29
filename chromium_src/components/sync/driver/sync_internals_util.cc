/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/driver/sync_internals_util.h"

#include "brave/components/sync/driver/brave_sync_service_impl.h"
#include "components/os_crypt/os_crypt.h"

#define ConstructAboutInformation ConstructAboutInformation_ChromiumImpl
#include "src/components/sync/driver/sync_internals_util.cc"
#undef ConstructAboutInformation

namespace syncer {

namespace sync_ui_util {

std::unique_ptr<base::DictionaryValue> ConstructAboutInformation(
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

  base::Value* details = about_info->GetDict().Find(kDetailsKey);
  DCHECK_NE(details, nullptr);
  DCHECK(details->is_list());

  details->Append(section_brave_sync.ToValue());

  return about_info;
}

}  // namespace sync_ui_util

}  // namespace syncer

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/chrome_sync_client.h"

#include "build/buildflag.h"
#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/sync/trusted_vault_client_android.h"
#endif
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/sync/driver/trusted_vault_client.h"
#include "extensions/buildflags/buildflags.h"

namespace {

struct NoOpTrustedVaultClient : syncer::TrustedVaultClient {
  void AddObserver(Observer* observer) override {}

  void RemoveObserver(Observer* observer) override {}

  void FetchKeys(
      const CoreAccountInfo& account_info,
      base::OnceCallback<void(const std::vector<std::vector<uint8_t>>&)> cb)
      override {}

  void MarkLocalKeysAsStale(const CoreAccountInfo& account_info,
                            base::OnceCallback<void(bool)> cb) override {}

  void StoreKeys(const std::string& gaia_id,
                 const std::vector<std::vector<uint8_t>>& keys,
                 int last_key_version) override {}

  void GetIsRecoverabilityDegraded(const CoreAccountInfo& account_info,
                                   base::OnceCallback<void(bool)> cb) override {
  }

  void AddTrustedRecoveryMethod(const std::string& gaia_id,
                                const std::vector<uint8_t>& public_key,
                                int method_type_hint,
                                base::OnceClosure cb) override {}

  void ClearDataForAccount(const CoreAccountInfo& account_info) override {}
};

}  // namespace

#if !BUILDFLAG(ENABLE_EXTENSIONS)

#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_SYNC_BRIDGE_H_

#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

#define TrustedVaultClientAndroid NoOpTrustedVaultClient

// Default enabled type is Bookmarks.
#define OnLocalSyncTransportDataCleared                                       \
  SetDefaultEnabledTypes(syncer::SyncService* sync_service) {                 \
    DCHECK(sync_service);                                                     \
                                                                              \
    syncer::UserSelectableTypeSet selected_types;                             \
    selected_types.Put(syncer::UserSelectableType::kBookmarks);               \
    sync_service->GetUserSettings()->SetSelectedTypes(false, selected_types); \
  }                                                                           \
                                                                              \
  void ChromeSyncClient::OnLocalSyncTransportDataCleared

#include "src/chrome/browser/sync/chrome_sync_client.cc"
#undef OnLocalSyncTransportDataCleared
#undef TrustedVaultClientAndroid

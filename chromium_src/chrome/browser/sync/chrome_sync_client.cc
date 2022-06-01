/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/buildflag.h"
#include "extensions/buildflags/buildflags.h"

#include <string>
#include <vector>
#include "base/callback.h"
#include "chrome/browser/sync/trusted_vault_client_android.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/sync/driver/trusted_vault_client.h"

#if !BUILDFLAG(ENABLE_EXTENSIONS)

#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_SYNC_BRIDGE_H_

#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

namespace tricks {
extern bool g_rewards_sync_initializing;
}

class BraveAndroidTrickyTrustedVaultClient : public syncer::TrustedVaultClient {
 public:
  BraveAndroidTrickyTrustedVaultClient() {
    if (!tricks::g_rewards_sync_initializing) {
      trusted_vault_client_ = std::make_unique<TrustedVaultClientAndroid>();
    }
  }
  ~BraveAndroidTrickyTrustedVaultClient() override {}

  BraveAndroidTrickyTrustedVaultClient(
      const BraveAndroidTrickyTrustedVaultClient&) = delete;
  BraveAndroidTrickyTrustedVaultClient& operator=(
      const BraveAndroidTrickyTrustedVaultClient&) = delete;

  // TrustedVaultClient implementation.
  void AddObserver(Observer* observer) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->AddObserver(observer);
    }
  }
  void RemoveObserver(Observer* observer) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->RemoveObserver(observer);
    }
  }
  void FetchKeys(
      const CoreAccountInfo& account_info,
      base::OnceCallback<void(const std::vector<std::vector<uint8_t>>&)> cb)
      override {
    if (trusted_vault_client_) {
      trusted_vault_client_->FetchKeys(account_info, std::move(cb));
    }
  }
  void StoreKeys(const std::string& gaia_id,
                 const std::vector<std::vector<uint8_t>>& keys,
                 int last_key_version) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->StoreKeys(gaia_id, keys, last_key_version);
    }
  }
  void MarkLocalKeysAsStale(const CoreAccountInfo& account_info,
                            base::OnceCallback<void(bool)> cb) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->MarkLocalKeysAsStale(account_info, std::move(cb));
    }
  }
  void GetIsRecoverabilityDegraded(const CoreAccountInfo& account_info,
                                   base::OnceCallback<void(bool)> cb) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->GetIsRecoverabilityDegraded(account_info,
                                                         std::move(cb));
    }
  }
  void AddTrustedRecoveryMethod(const std::string& gaia_id,
                                const std::vector<uint8_t>& public_key,
                                int method_type_hint,
                                base::OnceClosure cb) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->AddTrustedRecoveryMethod(
          gaia_id, public_key, method_type_hint, std::move(cb));
    }
  }
  void ClearDataForAccount(const CoreAccountInfo& account_info) override {
    if (trusted_vault_client_) {
      trusted_vault_client_->ClearDataForAccount(account_info);
    }
  }

 private:
  std::unique_ptr<syncer::TrustedVaultClient> trusted_vault_client_;
};

#define TrustedVaultClientAndroid BraveAndroidTrickyTrustedVaultClient
#include "src/chrome/browser/sync/chrome_sync_client.cc"
#undef TrustedVaultClientAndroid

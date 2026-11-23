/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_ACCOUNT_PERMISSION_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_ACCOUNT_PERMISSION_CONTEXT_H_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "components/permissions/object_permission_context_base.h"

class HostContentSettingsMap;

namespace brave_wallet {
class BraveWalletService;
}  // namespace brave_wallet

namespace url {
class Origin;
}

namespace permissions {

// Stores the Brave Wallet accounts each site has been granted, as objects of
// ContentSettingsType::BRAVE_WALLET_ACCOUNTS keyed on the site's origin. Each
// object holds only the account's `unique_key`.
class BraveWalletAccountPermissionContext : public ObjectPermissionContextBase {
 public:
  BraveWalletAccountPermissionContext(
      HostContentSettingsMap* map,
      brave_wallet::BraveWalletService* wallet_service);
  ~BraveWalletAccountPermissionContext() override;

  BraveWalletAccountPermissionContext(
      const BraveWalletAccountPermissionContext&) = delete;
  BraveWalletAccountPermissionContext& operator=(
      const BraveWalletAccountPermissionContext&) = delete;

  bool HasAccount(const url::Origin& origin,
                  const brave_wallet::mojom::AccountIdPtr& account_id);
  void GrantAccount(const url::Origin& origin,
                    const brave_wallet::mojom::AccountIdPtr& account_id);
  void RevokeAccount(const url::Origin& origin,
                     const brave_wallet::mojom::AccountIdPtr& account_id);

  std::vector<brave_wallet::mojom::AccountIdPtr> GetGrantedAccounts(
      const url::Origin& origin,
      brave_wallet::mojom::CoinType coin);
  std::vector<std::pair<url::Origin, brave_wallet::mojom::AccountIdPtr>>
  GetAllGrantedAccounts(brave_wallet::mojom::CoinType coin);

  // ObjectPermissionContextBase:
  std::string GetKeyForObject(const base::DictValue& object) override;
  bool IsValidObject(const base::DictValue& object) override;
  std::u16string GetObjectDisplayName(const base::DictValue& object) override;

 private:
  // The wallet's account for `unique_key`, or null when there is no wallet
  // service or no such account. Valid only until the keyring changes, so use
  // it right away.
  const brave_wallet::mojom::AccountInfo* FindAccountInfo(
      std::string_view unique_key) const;

  const raw_ptr<brave_wallet::BraveWalletService> wallet_service_;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_ACCOUNT_PERMISSION_CONTEXT_H_

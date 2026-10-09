/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXT_H_
#define BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXT_H_

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/permissions/permission_origin_lifetime_monitor.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/object_permission_context_base.h"
#include "url/origin.h"

class HostContentSettingsMap;

namespace permissions {

// Tracks which wallet accounts each origin may see, modelled on
// UsbChooserContext: a per-coin ask/block guard (BRAVE_ETHEREUM and friends)
// plus a per-coin website setting holding the granted accounts
// (BRAVE_ETHEREUM_CHOOSER_DATA and friends).
//
// One instance per coin is required, not merely conventional:
// ObjectPermissionContextBase::SaveWebsiteSetting rewrites an origin's entire
// object list, so two instances sharing a data type would clobber each other.
class BraveWalletAccountChooserContext : public ObjectPermissionContextBase {
 public:
  // `origin_lifetime_monitor` supplies the eTLD+1 teardown signal that ends
  // "until page close" grants. It may be null, in which case that lifetime is
  // refused rather than silently upgraded to a longer one.
  BraveWalletAccountChooserContext(
      brave_wallet::mojom::CoinType coin,
      HostContentSettingsMap* host_content_settings_map,
      std::unique_ptr<PermissionOriginLifetimeMonitor> origin_lifetime_monitor);

  BraveWalletAccountChooserContext(const BraveWalletAccountChooserContext&) =
      delete;
  BraveWalletAccountChooserContext& operator=(
      const BraveWalletAccountChooserContext&) = delete;

  ~BraveWalletAccountChooserContext() override;

  // Returns std::nullopt for coins that have no wallet permission.
  static std::optional<ContentSettingsType> GuardTypeForCoin(
      brave_wallet::mojom::CoinType coin);
  static std::optional<ContentSettingsType> DataTypeForCoin(
      brave_wallet::mojom::CoinType coin);

  // `lifetime` follows PermissionLifetimeOption: std::nullopt grants
  // permanently, a zero delta grants until the eTLD+1 is closed (held in
  // memory only), and a positive delta stores an expiry alongside the grant.
  void GrantAccountPermission(const url::Origin& origin,
                              std::string_view account,
                              std::optional<base::TimeDelta> lifetime);

  bool HasAccountPermission(const url::Origin& origin,
                            std::string_view account);

  // Granted, unexpired accounts for `origin`, in the form they were granted.
  // Empty when the guard is blocked.
  std::vector<std::string> GetGrantedAccounts(const url::Origin& origin);

  void RevokeAccountPermission(const url::Origin& origin,
                               std::string_view account);

  // Drops every grant for `origin`, persistent and ephemeral. Returns whether
  // anything was actually revoked.
  bool RevokeAllAccountPermissionsForOrigin(const url::Origin& origin);

  // Origins holding at least one unexpired grant.
  std::vector<url::Origin> GetOriginsWithGrantedAccounts();

  // Drops `account` from every origin, including origins whose guard is
  // currently blocked. Used when an account is removed from the wallet, so a
  // later unblock cannot resurrect the grant.
  void RevokeAccountPermissionForAllOrigins(std::string_view account);

  // Drops every grant for every origin, both persistent and ephemeral.
  void RevokeAllAccountPermissions();

  // ObjectPermissionContextBase:
  std::vector<std::unique_ptr<Object>> GetGrantedObjects(
      const url::Origin& origin) override;
  std::string GetKeyForObject(const base::DictValue& object) override;
  bool IsValidObject(const base::DictValue& object) override;
  std::u16string GetObjectDisplayName(const base::DictValue& object) override;

  // KeyedService:
  void Shutdown() override;

 private:
  struct EphemeralGrants {
    EphemeralGrants();
    EphemeralGrants(const EphemeralGrants&) = delete;
    EphemeralGrants& operator=(const EphemeralGrants&) = delete;
    EphemeralGrants(EphemeralGrants&&) noexcept;
    EphemeralGrants& operator=(EphemeralGrants&&) noexcept;
    ~EphemeralGrants();

    // Key handed back by PermissionOriginLifetimeMonitor, naming the ephemeral
    // storage area whose teardown ends these grants.
    std::string storage_domain;
    std::set<std::string> accounts;
  };

  // ObjectPermissionContextBase:
  std::vector<url::Origin> RevokeEphemeralPermissions(
      const ContentSettingsPattern& primary_pattern,
      bool unconditional) override;

  // Runs when an ephemeral storage area is torn down, i.e. the last tab for
  // that eTLD+1 closed.
  void OnPermissionOriginDestroyed(const std::string& storage_domain);

  // ETH addresses are checksummed, so their case carries no meaning and the
  // pre-chooser implementation compared them case-insensitively. SOL and ADA
  // identifiers are case-significant and are matched exactly.
  std::string NormalizeAccount(std::string_view account) const;

  void RevokeExpiredAccounts(const url::Origin& origin,
                             const std::vector<std::string>& keys);

  const brave_wallet::mojom::CoinType coin_;

  // Needed to reach origins the base class hides because their guard is
  // blocked; see RevokeAccountPermissionForAllOrigins. Annotated to match the
  // base class, which also outlives the map during its own destruction.
  const raw_ptr<HostContentSettingsMap, DanglingUntriaged>
      host_content_settings_map_;

  std::unique_ptr<PermissionOriginLifetimeMonitor> origin_lifetime_monitor_;

  // "Until page close" grants, which must outlive neither the eTLD+1 nor the
  // browser process and so are never written to the website setting.
  std::map<url::Origin, EphemeralGrants> ephemeral_accounts_;

  base::WeakPtrFactory<BraveWalletAccountChooserContext> weak_factory_{this};
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_CONTEXTS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXT_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_wallet_account_permission_context.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/origin.h"

namespace permissions {

namespace {

constexpr char kUniqueKeyKey[] = "unique_key";

const std::string* GetUniqueKey(const base::DictValue& object) {
  return object.FindString(kUniqueKeyKey);
}

std::string MakeAccountKey(
    const brave_wallet::mojom::AccountIdPtr& account_id) {
  CHECK(account_id);
  CHECK(!account_id->unique_key.empty());
  return account_id->unique_key;
}

base::DictValue MakeAccountObject(
    const brave_wallet::mojom::AccountIdPtr& account_id) {
  base::DictValue object;
  object.Set(kUniqueKeyKey, MakeAccountKey(account_id));
  return object;
}

}  // namespace

BraveWalletAccountPermissionContext::BraveWalletAccountPermissionContext(
    HostContentSettingsMap* map,
    brave_wallet::BraveWalletService* wallet_service)
    : ObjectPermissionContextBase(ContentSettingsType::BRAVE_WALLET_ACCOUNTS,
                                  map),
      wallet_service_(wallet_service) {}

BraveWalletAccountPermissionContext::~BraveWalletAccountPermissionContext() =
    default;

bool BraveWalletAccountPermissionContext::HasAccount(
    const url::Origin& origin,
    const brave_wallet::mojom::AccountIdPtr& account_id) {
  return GetGrantedObject(origin, MakeAccountKey(account_id)) != nullptr;
}

void BraveWalletAccountPermissionContext::GrantAccount(
    const url::Origin& origin,
    const brave_wallet::mojom::AccountIdPtr& account_id) {
  // TODO(https://github.com/brave/brave-browser/issues/61873): Record the
  // grant's lifetime with the permission lifetime manager. Grants do not
  // expire until then.
  GrantObjectPermission(origin, MakeAccountObject(account_id));
}

void BraveWalletAccountPermissionContext::RevokeAccount(
    const url::Origin& origin,
    const brave_wallet::mojom::AccountIdPtr& account_id) {
  RevokeObjectPermission(origin, MakeAccountKey(account_id));
}

std::vector<brave_wallet::mojom::AccountIdPtr>
BraveWalletAccountPermissionContext::GetGrantedAccounts(
    const url::Origin& origin,
    brave_wallet::mojom::CoinType coin) {
  std::vector<brave_wallet::mojom::AccountIdPtr> accounts;
  for (const auto& object : GetGrantedObjects(origin)) {
    const std::string* unique_key = GetUniqueKey(object->value);
    const brave_wallet::mojom::AccountInfo* account_info =
        unique_key ? FindAccountInfo(*unique_key) : nullptr;
    if (account_info && account_info->account_id->coin == coin) {
      accounts.push_back(account_info->account_id->Clone());
    }
  }
  return accounts;
}

std::vector<std::pair<url::Origin, brave_wallet::mojom::AccountIdPtr>>
BraveWalletAccountPermissionContext::GetAllGrantedAccounts(
    brave_wallet::mojom::CoinType coin) {
  std::vector<std::pair<url::Origin, brave_wallet::mojom::AccountIdPtr>> grants;
  for (const auto& object : GetAllGrantedObjects()) {
    const std::string* unique_key = GetUniqueKey(object->value);
    const brave_wallet::mojom::AccountInfo* account_info =
        unique_key ? FindAccountInfo(*unique_key) : nullptr;
    if (account_info && account_info->account_id->coin == coin) {
      grants.emplace_back(url::Origin::Create(object->origin),
                          account_info->account_id->Clone());
    }
  }
  return grants;
}

std::string BraveWalletAccountPermissionContext::GetKeyForObject(
    const base::DictValue& object) {
  const std::string* unique_key = GetUniqueKey(object);
  CHECK(unique_key);
  return *unique_key;
}

bool BraveWalletAccountPermissionContext::IsValidObject(
    const base::DictValue& object) {
  const std::string* unique_key = GetUniqueKey(object);
  return unique_key && !unique_key->empty();
}

std::u16string BraveWalletAccountPermissionContext::GetObjectDisplayName(
    const base::DictValue& object) {
  const std::string* unique_key = GetUniqueKey(object);
  if (!unique_key) {
    return std::u16string();
  }
  // An account the wallet does not know is shown by its key, so the entry is
  // not blank.
  if (const brave_wallet::mojom::AccountInfo* account_info =
          FindAccountInfo(*unique_key)) {
    return base::UTF8ToUTF16(account_info->name);
  }
  return base::UTF8ToUTF16(*unique_key);
}

const brave_wallet::mojom::AccountInfo*
BraveWalletAccountPermissionContext::FindAccountInfo(
    std::string_view unique_key) const {
  if (!wallet_service_) {
    return nullptr;
  }
  for (const auto& account_info :
       wallet_service_->keyring_service()->GetAllAccountInfos()) {
    if (account_info->account_id->unique_key == unique_key) {
      return account_info.get();
    }
  }
  return nullptr;
}

}  // namespace permissions

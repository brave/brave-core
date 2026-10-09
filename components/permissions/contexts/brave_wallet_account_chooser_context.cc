/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_wallet_account_chooser_context.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "url/gurl.h"

namespace permissions {

namespace {

constexpr char kAccountKey[] = "account";
constexpr char kExpiryKey[] = "expiry";

// Matches ObjectPermissionContextBase's own storage key.
constexpr char kObjectListKey[] = "chosen-objects";

base::DictValue MakeAccountObject(std::string_view account,
                                  std::optional<base::Time> expiry) {
  base::DictValue object;
  object.Set(kAccountKey, account);
  if (expiry) {
    object.Set(kExpiryKey, base::TimeToValue(*expiry));
  }
  return object;
}

std::optional<base::Time> GetExpiry(const base::DictValue& object) {
  return base::ValueToTime(object.Find(kExpiryKey));
}

bool IsExpired(const base::DictValue& object, base::Time now) {
  const auto expiry = GetExpiry(object);
  return expiry && *expiry <= now;
}

// The base class initializer list runs before any body, so resolve the types
// through CHECKing wrappers rather than dereferencing the optionals blind.
ContentSettingsType GuardTypeForCoinOrDie(brave_wallet::mojom::CoinType coin) {
  const auto type = BraveWalletAccountChooserContext::GuardTypeForCoin(coin);
  CHECK(type) << "No wallet permission guard for coin "
              << static_cast<int>(coin);
  return *type;
}

ContentSettingsType DataTypeForCoinOrDie(brave_wallet::mojom::CoinType coin) {
  const auto type = BraveWalletAccountChooserContext::DataTypeForCoin(coin);
  CHECK(type) << "No wallet account storage for coin "
              << static_cast<int>(coin);
  return *type;
}

}  // namespace

BraveWalletAccountChooserContext::EphemeralGrants::EphemeralGrants() = default;
BraveWalletAccountChooserContext::EphemeralGrants::EphemeralGrants(
    EphemeralGrants&&) noexcept = default;
BraveWalletAccountChooserContext::EphemeralGrants&
BraveWalletAccountChooserContext::EphemeralGrants::operator=(
    EphemeralGrants&&) noexcept = default;
BraveWalletAccountChooserContext::EphemeralGrants::~EphemeralGrants() = default;

BraveWalletAccountChooserContext::BraveWalletAccountChooserContext(
    brave_wallet::mojom::CoinType coin,
    HostContentSettingsMap* host_content_settings_map,
    std::unique_ptr<PermissionOriginLifetimeMonitor> origin_lifetime_monitor)
    : ObjectPermissionContextBase(GuardTypeForCoinOrDie(coin),
                                  DataTypeForCoinOrDie(coin),
                                  host_content_settings_map),
      coin_(coin),
      host_content_settings_map_(host_content_settings_map),
      origin_lifetime_monitor_(std::move(origin_lifetime_monitor)) {
  CHECK(host_content_settings_map_);

  // Ephemeral grants live only in memory, so unlike PermissionLifetimeManager
  // there is no persisted state to sweep at startup.
  if (origin_lifetime_monitor_) {
    origin_lifetime_monitor_->SetOnPermissionOriginDestroyedCallback(
        base::BindRepeating(
            &BraveWalletAccountChooserContext::OnPermissionOriginDestroyed,
            base::Unretained(this)));
  }
}

BraveWalletAccountChooserContext::~BraveWalletAccountChooserContext() = default;

void BraveWalletAccountChooserContext::Shutdown() {
  // The monitor holds a BrowserContext pointer, so drop it before teardown.
  origin_lifetime_monitor_.reset();
  ephemeral_accounts_.clear();
  ObjectPermissionContextBase::Shutdown();
  // Matches UsbChooserContext: persist queued writes while the content
  // settings map is still usable.
  FlushScheduledSaveSettingsCalls();
}

// static
std::optional<ContentSettingsType>
BraveWalletAccountChooserContext::GuardTypeForCoin(
    brave_wallet::mojom::CoinType coin) {
  switch (coin) {
    case brave_wallet::mojom::CoinType::ETH:
      return ContentSettingsType::BRAVE_ETHEREUM;
    case brave_wallet::mojom::CoinType::SOL:
      return ContentSettingsType::BRAVE_SOLANA;
    case brave_wallet::mojom::CoinType::ADA:
      return ContentSettingsType::BRAVE_CARDANO;
    default:
      return std::nullopt;
  }
}

// static
std::optional<ContentSettingsType>
BraveWalletAccountChooserContext::DataTypeForCoin(
    brave_wallet::mojom::CoinType coin) {
  switch (coin) {
    case brave_wallet::mojom::CoinType::ETH:
      return ContentSettingsType::BRAVE_ETHEREUM_CHOOSER_DATA;
    case brave_wallet::mojom::CoinType::SOL:
      return ContentSettingsType::BRAVE_SOLANA_CHOOSER_DATA;
    case brave_wallet::mojom::CoinType::ADA:
      return ContentSettingsType::BRAVE_CARDANO_CHOOSER_DATA;
    default:
      return std::nullopt;
  }
}

std::string BraveWalletAccountChooserContext::NormalizeAccount(
    std::string_view account) const {
  if (coin_ == brave_wallet::mojom::CoinType::ETH) {
    return base::ToLowerASCII(account);
  }
  return std::string(account);
}

void BraveWalletAccountChooserContext::GrantAccountPermission(
    const url::Origin& origin,
    std::string_view account,
    std::optional<base::TimeDelta> lifetime) {
  if (account.empty() || origin.opaque()) {
    return;
  }

  // A zero lifetime means "until the eTLD+1 is closed", which must not reach
  // the website setting.
  if (lifetime && lifetime->is_zero()) {
    if (!origin_lifetime_monitor_) {
      return;
    }
    // An empty key means no live ephemeral storage area will ever be torn down
    // for this origin, so nothing would revoke the grant. Refuse it rather
    // than hand out one that never expires.
    std::string storage_domain =
        origin_lifetime_monitor_->SubscribeToPermissionOriginDestruction(
            origin.GetURL());
    if (storage_domain.empty()) {
      return;
    }
    auto& grants = ephemeral_accounts_[origin];
    grants.storage_domain = std::move(storage_domain);
    grants.accounts.emplace(NormalizeAccount(account));
    NotifyPermissionChanged();
    return;
  }

  // A persistent grant supersedes an ephemeral one for the same account.
  const auto it = ephemeral_accounts_.find(origin);
  if (it != ephemeral_accounts_.end()) {
    it->second.accounts.erase(NormalizeAccount(account));
    if (it->second.accounts.empty()) {
      ephemeral_accounts_.erase(it);
    }
  }

  std::optional<base::Time> expiry;
  if (lifetime) {
    expiry = base::Time::Now() + *lifetime;
  }
  GrantObjectPermission(origin, MakeAccountObject(account, expiry));
}

bool BraveWalletAccountChooserContext::HasAccountPermission(
    const url::Origin& origin,
    std::string_view account) {
  if (account.empty()) {
    return false;
  }
  const std::string key = NormalizeAccount(account);
  const auto granted = GetGrantedAccounts(origin);
  return std::ranges::any_of(granted, [&](const std::string& granted_account) {
    return NormalizeAccount(granted_account) == key;
  });
}

std::vector<std::string> BraveWalletAccountChooserContext::GetGrantedAccounts(
    const url::Origin& origin) {
  std::vector<std::string> accounts;
  for (const auto& object : GetGrantedObjects(origin)) {
    if (const auto* account = object->value.FindString(kAccountKey)) {
      accounts.push_back(*account);
    }
  }
  return accounts;
}

void BraveWalletAccountChooserContext::RevokeAccountPermission(
    const url::Origin& origin,
    std::string_view account) {
  const std::string key = NormalizeAccount(account);

  const auto it = ephemeral_accounts_.find(origin);
  if (it != ephemeral_accounts_.end() && it->second.accounts.erase(key)) {
    if (it->second.accounts.empty()) {
      ephemeral_accounts_.erase(it);
    }
    NotifyPermissionRevoked(origin);
  }

  RevokeObjectPermission(origin, key);
}

bool BraveWalletAccountChooserContext::RevokeAllAccountPermissionsForOrigin(
    const url::Origin& origin) {
  const bool had_ephemeral = ephemeral_accounts_.erase(origin) > 0;
  // RevokeObjectPermissions notifies on its own when it removes anything.
  if (RevokeObjectPermissions(origin)) {
    return true;
  }
  if (had_ephemeral) {
    NotifyPermissionRevoked(origin);
  }
  return had_ephemeral;
}

std::vector<url::Origin>
BraveWalletAccountChooserContext::GetOriginsWithGrantedAccounts() {
  std::vector<url::Origin> origins;
  for (const auto& origin : GetOriginsWithGrants()) {
    if (!GetGrantedAccounts(origin).empty()) {
      origins.push_back(origin);
    }
  }
  return origins;
}

void BraveWalletAccountChooserContext::RevokeAccountPermissionForAllOrigins(
    std::string_view account) {
  if (account.empty()) {
    return;
  }
  const std::string key = NormalizeAccount(account);

  std::erase_if(ephemeral_accounts_, [&](auto& entry) {
    entry.second.accounts.erase(key);
    return entry.second.accounts.empty();
  });

  // Drain queued writes before snapshotting the stored settings below, so a
  // pending save cannot re-persist a cached list that still holds `account`.
  FlushScheduledSaveSettingsCalls();

  // Walk the stored setting rather than the base class cache: the cache omits
  // origins whose guard is blocked, and leaving a grant behind for one of them
  // would resurrect it if the user ever unblocks the origin.
  for (const ContentSettingPatternSource& setting :
       host_content_settings_map_->GetSettingsForOneType(
           data_content_settings_type_)) {
    const GURL origin_url(setting.primary_pattern.ToString());
    if (!origin_url.is_valid()) {
      continue;
    }

    const base::DictValue* setting_dict = setting.setting_value.GetIfDict();
    if (!setting_dict) {
      continue;
    }
    const base::ListValue* object_list = setting_dict->FindList(kObjectListKey);
    if (!object_list) {
      continue;
    }

    base::ListValue remaining;
    for (const auto& object : *object_list) {
      const auto* object_dict = object.GetIfDict();
      if (!object_dict) {
        continue;
      }
      const auto* stored = object_dict->FindString(kAccountKey);
      if (stored && NormalizeAccount(*stored) == key) {
        continue;
      }
      remaining.Append(object.Clone());
    }

    if (remaining.size() == object_list->size()) {
      continue;
    }

    base::Value new_setting;
    if (!remaining.empty()) {
      new_setting = base::Value(
          base::DictValue().Set(kObjectListKey, std::move(remaining)));
    }
    host_content_settings_map_->SetWebsiteSettingDefaultScope(
        origin_url, GURL(), data_content_settings_type_,
        std::move(new_setting));
  }
}

void BraveWalletAccountChooserContext::RevokeAllAccountPermissions() {
  ephemeral_accounts_.clear();
  // As above: a queued save would otherwise land after the clear.
  FlushScheduledSaveSettingsCalls();
  host_content_settings_map_->ClearSettingsForOneType(
      data_content_settings_type_);
}

std::vector<std::unique_ptr<ObjectPermissionContextBase::Object>>
BraveWalletAccountChooserContext::GetGrantedObjects(const url::Origin& origin) {
  auto objects = ObjectPermissionContextBase::GetGrantedObjects(origin);

  const base::Time now = base::Time::Now();
  std::vector<std::string> expired_keys;
  std::erase_if(objects, [&](const std::unique_ptr<Object>& object) {
    if (!IsExpired(object->value, now)) {
      return false;
    }
    expired_keys.push_back(GetKeyForObject(object->value));
    return true;
  });

  // Prune out of band: revoking here would notify observers in the middle of a
  // read.
  if (!expired_keys.empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&BraveWalletAccountChooserContext::RevokeExpiredAccounts,
                       weak_factory_.GetWeakPtr(), origin,
                       std::move(expired_keys)));
  }

  if (CanRequestObjectPermission(origin)) {
    const auto it = ephemeral_accounts_.find(origin);
    if (it != ephemeral_accounts_.end()) {
      for (const auto& account : it->second.accounts) {
        objects.push_back(std::make_unique<Object>(
            origin, MakeAccountObject(account, /*expiry=*/std::nullopt),
            content_settings::SettingSource::kUser, IsOffTheRecord()));
      }
    }
  }

  return objects;
}

void BraveWalletAccountChooserContext::RevokeExpiredAccounts(
    const url::Origin& origin,
    const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    // Re-check: the grant may have been renewed before this task ran.
    const auto object = GetGrantedObject(origin, key);
    if (object && IsExpired(object->value, base::Time::Now())) {
      RevokeObjectPermission(origin, key);
    }
  }
}

void BraveWalletAccountChooserContext::OnPermissionOriginDestroyed(
    const std::string& storage_domain) {
  std::vector<url::Origin> revoked_origins;
  std::erase_if(ephemeral_accounts_, [&](const auto& entry) {
    if (entry.second.storage_domain != storage_domain) {
      return false;
    }
    revoked_origins.push_back(entry.first);
    return true;
  });

  for (const auto& origin : revoked_origins) {
    NotifyPermissionRevoked(origin);
  }
}

std::vector<url::Origin>
BraveWalletAccountChooserContext::RevokeEphemeralPermissions(
    const ContentSettingsPattern& primary_pattern,
    bool unconditional) {
  std::vector<url::Origin> revoked_origins;
  std::erase_if(ephemeral_accounts_, [&](const auto& entry) {
    const auto& origin = entry.first;
    if (primary_pattern.Matches(origin.GetURL()) &&
        (unconditional || !CanRequestObjectPermission(origin))) {
      revoked_origins.push_back(origin);
      return true;
    }
    return false;
  });
  return revoked_origins;
}

std::string BraveWalletAccountChooserContext::GetKeyForObject(
    const base::DictValue& object) {
  const auto* account = object.FindString(kAccountKey);
  return account ? NormalizeAccount(*account) : std::string();
}

bool BraveWalletAccountChooserContext::IsValidObject(
    const base::DictValue& object) {
  const auto* account = object.FindString(kAccountKey);
  if (!account || account->empty()) {
    return false;
  }
  // `expiry` is optional, but a malformed one must not be read as "permanent".
  const bool has_expiry = object.contains(kExpiryKey);
  if (has_expiry && !GetExpiry(object)) {
    return false;
  }
  // Reject anything carrying unknown keys.
  return object.size() == (has_expiry ? 2u : 1u);
}

std::u16string BraveWalletAccountChooserContext::GetObjectDisplayName(
    const base::DictValue& object) {
  const auto* account = object.FindString(kAccountKey);
  return account ? base::UTF8ToUTF16(*account) : std::u16string();
}

}  // namespace permissions

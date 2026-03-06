/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_service.h"

#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_utils.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

namespace {

constexpr char kOriginSkuHostnamePart[] = "origin";
constexpr char kRemainingCredentialCount[] = "remaining_credential_count";
constexpr char kExpiresAt[] = "expires_at";

// Helper function to check if a policy is controlled by BraveOrigin in a given
// policy service
bool IsPolicyControlledByBraveOrigin(policy::PolicyService* policy_service,
                                     std::string_view policy_key) {
  if (!policy_service) {
    return false;
  }

  const policy::PolicyMap& policies = policy_service->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string()));
  const policy::PolicyMap::Entry* entry = policies.Get(std::string(policy_key));
  return entry && entry->source == policy::POLICY_SOURCE_BRAVE;
}
}  // namespace

BraveOriginService::BraveOriginService(
    PrefService* local_state,
    PrefService* profile_prefs,
    std::string_view profile_id,
    policy::PolicyService* profile_policy_service,
    policy::PolicyService* browser_policy_service,
    SkusServiceGetter skus_service_getter)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      profile_id_(profile_id),
      profile_policy_service_(profile_policy_service),
      browser_policy_service_(browser_policy_service),
      skus_service_getter_(std::move(skus_service_getter)),
      origin_sku_domain_(
          brave_domains::GetServicesDomain(kOriginSkuHostnamePart)) {
  CHECK(local_state_);
  CHECK(profile_prefs_);
  CHECK(!profile_id_.empty());

  // Eagerly check purchase state on startup so the cached value is available.
  CheckPurchaseState(base::DoNothing());
}

BraveOriginService::~BraveOriginService() = default;

void BraveOriginService::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  skus_service_.reset();
  local_state_ = nullptr;
  profile_prefs_ = nullptr;
  profile_policy_service_ = nullptr;
  browser_policy_service_ = nullptr;
}

bool BraveOriginService::IsPolicyControlledByBraveOrigin(
    std::string_view policy_key) const {
  if (!IsBraveOriginEnabled()) {
    return false;
  }

  // Check if this is a valid BraveOrigin policy
  const BraveOriginPolicyInfo* policy_info =
      BraveOriginPolicyManager::GetInstance()->GetPolicyInfo(policy_key);
  if (!policy_info) {
    return false;
  }

  // Check if the policy is controlled by BraveOrigin in either browser or
  // profile policy service
  return ::brave_origin::IsPolicyControlledByBraveOrigin(
             browser_policy_service_, policy_key) ||
         ::brave_origin::IsPolicyControlledByBraveOrigin(
             profile_policy_service_, policy_key);
}

bool BraveOriginService::SetPolicyValue(std::string_view policy_key,
                                        bool value) {
  if (!IsBraveOriginEnabled()) {
    return false;
  }

  // Get policy info to access pref_name and user_settable
  auto* manager = BraveOriginPolicyManager::GetInstance();
  const BraveOriginPolicyInfo* policy_info = manager->GetPolicyInfo(policy_key);
  if (!policy_info) {
    return false;
  }

  // Set the policy value in BraveOriginPolicyManager
  PrefService* target_prefs = nullptr;
  if (manager->IsBrowserPolicy(policy_key)) {
    manager->SetPolicyValue(policy_key, value);
    target_prefs = local_state_;
  } else if (manager->IsProfilePolicy(policy_key)) {
    manager->SetPolicyValue(policy_key, value, profile_id_);
    target_prefs = profile_prefs_;
  }
  CHECK(target_prefs);

  // Also set the corresponding pref value
  if (!policy_info->user_settable && value == policy_info->default_value) {
    target_prefs->ClearPref(policy_info->pref_name);
  } else {
    target_prefs->SetBoolean(policy_info->pref_name, value);
  }

  return true;
}

std::optional<bool> BraveOriginService::GetPolicyValue(
    std::string_view policy_key) const {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  if (manager->IsBrowserPolicy(policy_key)) {
    return manager->GetPolicyValue(policy_key);
  } else if (manager->IsProfilePolicy(policy_key)) {
    return manager->GetPolicyValue(policy_key, profile_id_);
  }
  return std::nullopt;
}

void BraveOriginService::CheckPurchaseState(
    base::OnceCallback<void(bool)> callback) {
  if (!EnsureSkusConnected()) {
    std::move(callback).Run(IsPurchased());
    return;
  }

  skus_service_->CredentialSummary(
      origin_sku_domain_,
      base::BindOnce(&BraveOriginService::OnCredentialSummary,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

bool BraveOriginService::IsPurchased() const {
  return BraveOriginPolicyManager::GetInstance()->IsPurchased();
}

void BraveOriginService::OnCredentialSummary(
    base::OnceCallback<void(bool)> callback,
    skus::mojom::SkusResultPtr summary) {
  bool purchased = false;

  std::string summary_trimmed;
  base::TrimWhitespaceASCII(summary->message, base::TrimPositions::TRIM_ALL,
                            &summary_trimmed);
  if (!summary_trimmed.empty()) {
    std::optional<base::DictValue> records = base::JSONReader::ReadDict(
        summary->message, base::JSONParserOptions::JSON_PARSE_RFC);

    if (records && !records->empty()) {
      // If there are remaining credentials or an expires_at field, the user
      // has a valid Origin purchase.
      int remaining = records->FindInt(kRemainingCredentialCount).value_or(0);
      const std::string* expires_at = records->FindString(kExpiresAt);
      purchased =
          remaining > 0 || (expires_at != nullptr && !expires_at->empty());
    }
  }

  BraveOriginPolicyManager::GetInstance()->SetPurchased(purchased);
  std::move(callback).Run(purchased);
}

bool BraveOriginService::EnsureSkusConnected() {
  if (!skus_service_) {
    if (!skus_service_getter_) {
      return false;
    }
    auto pending = skus_service_getter_.Run();
    if (pending.is_valid()) {
      skus_service_.Bind(std::move(pending));
      skus_service_.reset_on_disconnect();
    }
  }
  return !!skus_service_;
}

}  // namespace brave_origin

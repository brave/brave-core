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
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
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
    std::unique_ptr<Delegate> delegate)
    : local_state_(local_state),
      profile_prefs_(profile_prefs),
      profile_id_(profile_id),
      profile_policy_service_(profile_policy_service),
      browser_policy_service_(browser_policy_service),
      // STAGING for unofficial builds; official builds always resolve to prod.
      origin_sku_domain_(brave_domains::GetServicesDomain(
          kOriginSkuHostnamePart,
          brave_domains::ServicesEnvironment::STAGING)),
      delegate_(std::move(delegate)) {
  CHECK(local_state_);
  CHECK(profile_prefs_);
  CHECK(!profile_id_.empty());

#if BUILDFLAG(IS_LINUX)
  // On Linux, treat free tier acceptance as a valid purchase so policies
  // are applied immediately at startup without waiting for the SKU check.
  if (local_state_->GetBoolean(kOriginFreeTierAccepted)) {
    BraveOriginPolicyManager::GetInstance()->SetPurchased(true);
  }
#endif

  // Eagerly check purchase state on startup so the cached value is available.
  CheckPurchaseState(base::DoNothing());

  // Re-check purchase state whenever SKU credentials change (e.g. after
  // the user completes a purchase on account.brave.com).
  skus_pref_registrar_.Init(local_state_);
  skus_pref_registrar_.Add(
      skus::prefs::kSkusState,
      base::BindRepeating(&BraveOriginService::OnSkusStateChanged,
                          base::Unretained(this)));

  // Record whether Origin was enforcing policies in the previous session.
  startup_was_enforcing_ =
      local_state_->GetBoolean(kOriginPoliciesWereEnforced);

  // Snapshot policy values at construction time so NeedsRestart() can
  // detect settings changes later.
  auto* manager = BraveOriginPolicyManager::GetInstance();
  if (manager->IsInitialized()) {
    startup_browser_policies_ = manager->GetAllBrowserPolicies();
    startup_profile_policies_ = manager->GetAllProfilePolicies(profile_id_);
  }
}

BraveOriginService::~BraveOriginService() = default;

void BraveOriginService::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  skus_pref_registrar_.RemoveAll();
  delegate_.reset();
  skus_service_.reset();
  local_state_ = nullptr;
  profile_prefs_ = nullptr;
  profile_policy_service_ = nullptr;
  browser_policy_service_ = nullptr;
}

bool BraveOriginService::IsPolicyControlledByBraveOrigin(
    std::string_view policy_key) const {
  if (!IsBraveOriginPurchased()) {
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
  if (!IsBraveOriginPurchased()) {
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

bool BraveOriginService::NeedsRestart() const {
  auto* manager = BraveOriginPolicyManager::GetInstance();
  if (!manager->IsInitialized()) {
    return false;
  }

  // First purchase this session: policies are now enforced but weren't
  // in the previous session. startup_was_enforcing_ is read from the
  // persisted kOriginPoliciesWereEnforced pref (set by this service
  // when a purchase is confirmed via OnCredentialSummary).
  if (manager->IsPurchased() && !startup_was_enforcing_) {
    return true;
  }

  return manager->GetAllBrowserPolicies() != startup_browser_policies_ ||
         manager->GetAllProfilePolicies(profile_id_) !=
             startup_profile_policies_;
}

void BraveOriginService::OnSkusStateChanged() {
  // SKU credentials were updated (e.g. via the JS API on
  // account.brave.com). Re-check whether a purchase is now valid.
  CheckPurchaseState(base::DoNothing());
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

#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  // Check if this is a first-time purchase (pref was false, now true).
  // Must read the pref before SetPurchased() updates it. Only needed for
  // the upgrade case below; branded builds drive post-purchase via dialog.
  const bool was_previously_purchased =
      local_state_ && local_state_->GetBoolean(kOriginPurchaseValidated);
#endif

#if BUILDFLAG(IS_LINUX)
  // On Linux, free tier acceptance overrides the SKU result so that
  // policies remain active even when there is no purchase credential.
  if (!purchased && local_state_ &&
      local_state_->GetBoolean(kOriginFreeTierAccepted)) {
    purchased = true;
  }
#endif

  BraveOriginPolicyManager::GetInstance()->SetPurchased(purchased);

  // Persist enforcement state so NeedsRestart() can detect first-purchase
  // across sessions. This pref is read at next startup to distinguish
  // existing purchasers (no restart needed) from new ones.
  if (local_state_ && purchased) {
    local_state_->SetBoolean(kOriginPoliciesWereEnforced, true);
  }

#if !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  // On first purchase detection in the upgrade case, open the settings page
  // so the user can configure Origin policies and restart. Branded builds
  // handle the post-purchase flow via the startup dialog instead.
  if (purchased && !was_previously_purchased && delegate_ &&
      !did_open_origin_settings_) {
    delegate_->OpenOriginSettings();
    did_open_origin_settings_ = true;
  }
#endif

  std::move(callback).Run(purchased);
}

bool BraveOriginService::EnsureSkusConnected() {
  if (!skus_service_) {
    if (!delegate_) {
      return false;
    }
    auto pending = delegate_->GetSkusService();
    if (pending.is_valid()) {
      skus_service_.Bind(std::move(pending));
      skus_service_.reset_on_disconnect();
    }
  }
  return !!skus_service_;
}

#if BUILDFLAG(IS_LINUX)
void BraveOriginService::AcceptFreeTier() {
  if (local_state_) {
    local_state_->SetBoolean(kOriginFreeTierAccepted, true);
  }
  BraveOriginPolicyManager::GetInstance()->SetPurchased(true);
}

bool BraveOriginService::IsFreeTierAccepted() const {
  return local_state_ && local_state_->GetBoolean(kOriginFreeTierAccepted);
}
#endif

}  // namespace brave_origin

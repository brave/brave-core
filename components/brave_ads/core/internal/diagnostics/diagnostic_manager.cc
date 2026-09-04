/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_campaign_diagnostics_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_condition_matcher_diagnostics_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_confirmation_diagnostics_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_value_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_next_update_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_version_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/country_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/device_id_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/language_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_database_migration_failure_reason_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/new_tab_page_ads_schema_version_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/notification_ads_enabled_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/opted_into_new_tab_page_ads_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/opted_into_search_result_ads_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/optional_bool_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/permission_rule_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/resource_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/schema_version_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/browser_is_active_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/catalog_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/command_line_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/confirmation_tokens_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/do_not_disturb_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/full_screen_mode_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/issuers_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/media_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/network_connection_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/can_show_notifications_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/user_activity_permission_rule.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

namespace brave_ads {

namespace {

std::optional<bool> IsWalletValid() {
  const std::optional<WalletInfo>& wallet = GetAccount().GetWallet();
  if (!wallet) {
    return std::nullopt;
  }

  return wallet->IsValid();
}

std::optional<bool> AreIssuersValid() {
  const std::optional<IssuersInfo> issuers = GetIssuers();
  if (!issuers) {
    return std::nullopt;
  }

  return IsIssuersValid(*issuers);
}

bool IsTextClassificationResourceLoaded() {
  return GetAdHandler().GetTextClassificationResource().IsLoaded();
}

std::optional<std::string> GetTextClassificationResourceManifestVersion() {
  return GetAdHandler().GetTextClassificationResource().GetManifestVersion();
}

bool IsPurchaseIntentResourceLoaded() {
  return GetAdHandler().GetPurchaseIntentResource().IsLoaded();
}

std::optional<std::string> GetPurchaseIntentResourceManifestVersion() {
  return GetAdHandler().GetPurchaseIntentResource().GetManifestVersion();
}

bool IsAntiTargetingResourceLoaded() {
  return GetAdHandler().GetAntiTargetingResource().IsLoaded();
}

std::optional<std::string> GetAntiTargetingResourceManifestVersion() {
  return GetAdHandler().GetAntiTargetingResource().GetManifestVersion();
}

struct ResourceTableEntry final {
  DiagnosticEntryType type;
  const char* name;
  bool (*is_loaded)();
  std::optional<std::string> (*get_manifest_version)();
};

constexpr ResourceTableEntry kResourceTable[] = {
    {DiagnosticEntryType::kTextClassificationResource,
     "Text classification resource", &IsTextClassificationResourceLoaded,
     &GetTextClassificationResourceManifestVersion},
    {DiagnosticEntryType::kPurchaseIntentResource, "Purchase intent resource",
     &IsPurchaseIntentResourceLoaded,
     &GetPurchaseIntentResourceManifestVersion},
    {DiagnosticEntryType::kAntiTargetingResource, "Anti targeting resource",
     &IsAntiTargetingResourceLoaded, &GetAntiTargetingResourceManifestVersion}};

struct PermissionRuleTableEntry final {
  DiagnosticEntryType type;
  const char* name;
  bool (*has_permission)();
};

constexpr PermissionRuleTableEntry kPermissionRuleTable[] = {
    {DiagnosticEntryType::kCatalogPermission, "Catalog permission",
     &HasCatalogPermission},
    {DiagnosticEntryType::kNetworkConnectionPermission,
     "Network connection permission", &HasNetworkConnectionPermission},
    {DiagnosticEntryType::kBrowserIsActivePermission,
     "Browser is active permission", &HasBrowserIsActivePermission},
    {DiagnosticEntryType::kFullScreenModePermission,
     "Full screen mode permission", &HasFullScreenModePermission},
    {DiagnosticEntryType::kMediaPermission, "Media permission",
     &HasMediaPermission},
    {DiagnosticEntryType::kDoNotDisturbPermission, "Do not disturb permission",
     &HasDoNotDisturbPermission},
    {DiagnosticEntryType::kIssuersPermission, "Issuers permission",
     &HasIssuersPermission},
    {DiagnosticEntryType::kConfirmationTokensPermission,
     "Confirmation tokens permission", &HasConfirmationTokensPermission},
    {DiagnosticEntryType::kUserActivityPermission, "User activity permission",
     &HasUserActivityPermission},
    {DiagnosticEntryType::kCommandLinePermission, "Command line permission",
     &HasCommandLinePermission},
    {DiagnosticEntryType::kCanShowNotificationsPermission,
     "Can show notifications permission", &HasCanShowNotificationsPermission}};

// Entries below are reported through their own tab-specific getter instead
// of `GetDiagnostics`, so the General/Diagnostics tab doesn't show entries
// meant for another tab before that tab exists to display them.
constexpr auto kRewardsDiagnosticEntryTypes =
    base::MakeFixedFlatSet<DiagnosticEntryType>(
        {DiagnosticEntryType::kWalletValid,
         DiagnosticEntryType::kWalletConnected,
         DiagnosticEntryType::kIssuersValid});

constexpr auto kStorageDiagnosticEntryTypes =
    base::MakeFixedFlatSet<DiagnosticEntryType>(
        {DiagnosticEntryType::kSchemaVersion,
         DiagnosticEntryType::kLastDatabaseMigrationFailureReason});

constexpr auto kResourcesDiagnosticEntryTypes =
    base::MakeFixedFlatSet<DiagnosticEntryType>(
        {DiagnosticEntryType::kCatalogVersion,
         DiagnosticEntryType::kCatalogNextUpdate,
         DiagnosticEntryType::kNewTabPageAdsSchemaVersion,
         DiagnosticEntryType::kTextClassificationResource,
         DiagnosticEntryType::kPurchaseIntentResource,
         DiagnosticEntryType::kAntiTargetingResource});

constexpr auto kPermissionRulesDiagnosticEntryTypes =
    base::MakeFixedFlatSet<DiagnosticEntryType>(
        {DiagnosticEntryType::kCatalogPermission,
         DiagnosticEntryType::kNetworkConnectionPermission,
         DiagnosticEntryType::kBrowserIsActivePermission,
         DiagnosticEntryType::kFullScreenModePermission,
         DiagnosticEntryType::kMediaPermission,
         DiagnosticEntryType::kDoNotDisturbPermission,
         DiagnosticEntryType::kIssuersPermission,
         DiagnosticEntryType::kConfirmationTokensPermission,
         DiagnosticEntryType::kUserActivityPermission,
         DiagnosticEntryType::kCommandLinePermission,
         DiagnosticEntryType::kCanShowNotificationsPermission});

bool IsRewardsDiagnosticEntryType(DiagnosticEntryType type) {
  return kRewardsDiagnosticEntryTypes.contains(type);
}

bool IsStorageDiagnosticEntryType(DiagnosticEntryType type) {
  return kStorageDiagnosticEntryTypes.contains(type);
}

bool IsResourcesDiagnosticEntryType(DiagnosticEntryType type) {
  return kResourcesDiagnosticEntryTypes.contains(type);
}

bool IsPermissionRulesDiagnosticEntryType(DiagnosticEntryType type) {
  return kPermissionRulesDiagnosticEntryTypes.contains(type);
}

bool IsTabSpecificDiagnosticEntryType(DiagnosticEntryType type) {
  return IsRewardsDiagnosticEntryType(type) ||
         IsStorageDiagnosticEntryType(type) ||
         IsResourcesDiagnosticEntryType(type) ||
         IsPermissionRulesDiagnosticEntryType(type);
}

}  // namespace

DiagnosticManager::DiagnosticManager() {
  SetEntry(std::make_unique<OptionalBoolDiagnosticEntry>(
      DiagnosticEntryType::kWalletValid, "Wallet valid",
      base::BindRepeating(&IsWalletValid)));
  SetEntry(std::make_unique<PermissionRuleDiagnosticEntry>(
      DiagnosticEntryType::kWalletConnected, "Connected",
      base::BindRepeating(&UserHasJoinedBraveRewardsAndConnectedWallet)));
  SetEntry(std::make_unique<OptionalBoolDiagnosticEntry>(
      DiagnosticEntryType::kIssuersValid, "Issuers valid",
      base::BindRepeating(&AreIssuersValid)));
  SetEntry(std::make_unique<CatalogIdDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogVersionDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogLastUpdatedDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogNextUpdateDiagnosticEntry>());
  SetEntry(std::make_unique<DeviceIdDiagnosticEntry>());
  SetEntry(std::make_unique<LastUnIdleTimeDiagnosticEntry>());
  SetEntry(std::make_unique<NewTabPageAdsSchemaVersionDiagnosticEntry>());
  SetEntry(std::make_unique<LanguageDiagnosticEntry>());
  SetEntry(std::make_unique<CountryDiagnosticEntry>());
  SetEntry(std::make_unique<SchemaVersionDiagnosticEntry>());
  SetEntry(
      std::make_unique<LastDatabaseMigrationFailureReasonDiagnosticEntry>());
  SetEntry(std::make_unique<OptedInToNewTabPageAdsDiagnosticEntry>());
  SetEntry(std::make_unique<NotificationAdsEnabledDiagnosticEntry>());
  SetEntry(std::make_unique<OptedInToSearchResultAdsDiagnosticEntry>());
  for (const auto& entry : kResourceTable) {
    SetEntry(std::make_unique<ResourceDiagnosticEntry>(
        entry.type, entry.name, base::BindRepeating(entry.is_loaded),
        base::BindRepeating(entry.get_manifest_version)));
  }
  for (const auto& entry : kPermissionRuleTable) {
    SetEntry(std::make_unique<PermissionRuleDiagnosticEntry>(
        entry.type, entry.name, base::BindRepeating(entry.has_permission)));
  }
}

DiagnosticManager::~DiagnosticManager() = default;

// static
DiagnosticManager& DiagnosticManager::GetInstance() {
  return GlobalState::GetInstance()->GetDiagnosticManager();
}

void DiagnosticManager::SetEntry(
    std::unique_ptr<DiagnosticEntryInterface> entry) {
  CHECK(entry);

  const DiagnosticEntryType type = entry->GetType();
  diagnostics_[type] = std::move(entry);
}

void DiagnosticManager::GetDiagnostics(GetDiagnosticsCallback callback) const {
  std::move(callback).Run(DiagnosticsToList(
      diagnostics_, base::BindRepeating([](DiagnosticEntryType type) {
        return !IsTabSpecificDiagnosticEntryType(type);
      })));
}

void DiagnosticManager::GetRewardsDiagnostics(
    GetDiagnosticsCallback callback) const {
  std::move(callback).Run(DiagnosticsToList(
      diagnostics_, base::BindRepeating(&IsRewardsDiagnosticEntryType)));
}

void DiagnosticManager::GetStorageDiagnostics(
    GetDiagnosticsCallback callback) const {
  std::move(callback).Run(DiagnosticsToList(
      diagnostics_, base::BindRepeating(&IsStorageDiagnosticEntryType)));
}

void DiagnosticManager::GetResourcesDiagnostics(
    GetDiagnosticsCallback callback) const {
  std::move(callback).Run(DiagnosticsToList(
      diagnostics_, base::BindRepeating(&IsResourcesDiagnosticEntryType)));
}

void DiagnosticManager::GetPermissionRulesDiagnostics(
    GetDiagnosticsCallback callback) const {
  std::move(callback).Run(DiagnosticsToList(
      diagnostics_,
      base::BindRepeating(&IsPermissionRulesDiagnosticEntryType)));
}

void DiagnosticManager::GetConfirmationQueue(
    GetConfirmationQueueDiagnosticsCallback callback) {
  database::table::ConfirmationQueue database_table;
  database_table.GetAll(
      base::BindOnce(&GetConfirmationQueueCallback, std::move(callback)));
}

void DiagnosticManager::GetPaymentTokens(
    GetPaymentTokensDiagnosticsCallback callback) {
  database::table::PaymentTokens database_table;
  database_table.GetAll(
      base::BindOnce(&GetPaymentTokensCallback, std::move(callback)));
}

void DiagnosticManager::GetTransactions(
    GetTransactionsDiagnosticsCallback callback) {
  database::table::Transactions database_table;
  database_table.GetForDateRange(
      base::Time::Min(), base::Time::Max(),
      base::BindOnce(&GetTransactionsCallback, std::move(callback)));
}

void DiagnosticManager::GetNotificationAdCampaigns(
    GetCampaignsDiagnosticsCallback callback) {
  database::table::CreativeNotificationAds database_table;
  database_table.GetForActiveCampaigns(
      base::BindOnce(&GetNotificationAdCampaignsCallback, std::move(callback)));
}

void DiagnosticManager::GetNewTabPageAdCampaigns(
    GetCampaignsDiagnosticsCallback callback) {
  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForActiveCampaigns(
      base::BindOnce(&GetNewTabPageAdCampaignsCallback, std::move(callback)));
}

void DiagnosticManager::GetConditionMatchers(
    GetConditionMatchersDiagnosticsCallback callback) {
  // New Tab Page ads only; Notification ads' condition matchers only exist
  // at the catalog's creative set level in memory during parsing and aren't
  // persisted to the `creative_notification_ads` table.
  database::table::CreativeNewTabPageAds database_table;
  database_table.GetAll(
      base::BindOnce(&GetConditionMatchersCallback, std::move(callback)));
}

void DiagnosticManager::EvaluateConditionMatcher(
    const std::string& pref_path,
    const std::string& condition,
    std::optional<std::string> test_value,
    EvaluateConditionMatcherCallback callback) {
  const ConditionMatcherMap condition_matchers = {{pref_path, condition}};
  const base::flat_set<std::string> ad_event_virtual_pref_query_ids =
      GetAdEventVirtualPrefQueryIds(condition_matchers);

  database::table::AdEvents ad_events_database_table;
  ad_events_database_table.GetVirtualPrefs(
      ad_event_virtual_pref_query_ids,
      base::BindOnce(&BuildEvaluateConditionMatcherResultCallback,
                     std::move(callback), pref_path, condition,
                     std::move(test_value)));
}

}  // namespace brave_ads

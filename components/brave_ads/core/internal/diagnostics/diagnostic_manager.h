/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_types.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace base {
class ListValue;
}  // namespace base

namespace brave_ads {

// Confirmation queue contents for `brave://ads-internals`, reported alongside
// the synchronous `DiagnosticEntryInterface` entries but delivered via its
// own callback because reading it requires an async database query.
using GetConfirmationQueueDiagnosticsCallback =
    base::OnceCallback<void(bool success, base::ListValue confirmation_queue)>;

// Payment token contents for `brave://ads-internals`, reported alongside the
// synchronous `DiagnosticEntryInterface` entries but delivered via its own
// callback because reading it requires an async database query.
using GetPaymentTokensDiagnosticsCallback =
    base::OnceCallback<void(bool success, base::ListValue payment_tokens)>;

// Transaction history for `brave://ads-internals`, reported alongside the
// synchronous `DiagnosticEntryInterface` entries but delivered via its own
// callback because reading it requires an async database query.
using GetTransactionsDiagnosticsCallback =
    base::OnceCallback<void(bool success, base::ListValue transactions)>;

// One entry per currently active, region-targeted campaign (advertiser id,
// campaign id, start/end dates) for `brave://ads-internals`, alongside the
// total count of active, region-targeted creative ads across all of them.
// Computed in the same pass as the campaigns list so the caller doesn't
// need a second query to get both.
using GetCampaignsDiagnosticsCallback = base::OnceCallback<
    void(bool success, size_t active_ad_count, base::ListValue campaigns)>;

// One entry per condition matcher declared on every New Tab Page ad creative
// (active or not), reported alongside its currently resolved pref value, for
// `brave://ads-internals`.
using GetConditionMatchersDiagnosticsCallback =
    base::OnceCallback<void(bool success, base::ListValue condition_matchers)>;

class DiagnosticManager final {
 public:
  DiagnosticManager();

  DiagnosticManager(const DiagnosticManager&) = delete;
  DiagnosticManager& operator=(const DiagnosticManager&) = delete;

  ~DiagnosticManager();

  static DiagnosticManager& GetInstance();

  void SetEntry(std::unique_ptr<DiagnosticEntryInterface> entry);

  // Excludes entries returned by the tab-specific getters below.
  void GetDiagnostics(GetDiagnosticsCallback callback) const;

  void GetRewardsDiagnostics(GetDiagnosticsCallback callback) const;
  void GetStorageDiagnostics(GetDiagnosticsCallback callback) const;
  void GetResourcesDiagnostics(GetDiagnosticsCallback callback) const;
  void GetPermissionRulesDiagnostics(GetDiagnosticsCallback callback) const;

  static void GetConfirmationQueue(
      GetConfirmationQueueDiagnosticsCallback callback);

  static void GetPaymentTokens(GetPaymentTokensDiagnosticsCallback callback);

  static void GetTransactions(GetTransactionsDiagnosticsCallback callback);

  static void GetNotificationAdCampaigns(
      GetCampaignsDiagnosticsCallback callback);

  static void GetNewTabPageAdCampaigns(
      GetCampaignsDiagnosticsCallback callback);

  static void GetConditionMatchers(
      GetConditionMatchersDiagnosticsCallback callback);

  static void TestDiagnosticsConditionMatcher(
      const std::string& pref_path,
      const std::string& condition,
      std::optional<std::string> test_value,
      TestDiagnosticsConditionMatcherCallback callback);

 private:
  DiagnosticMap diagnostics_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_H_

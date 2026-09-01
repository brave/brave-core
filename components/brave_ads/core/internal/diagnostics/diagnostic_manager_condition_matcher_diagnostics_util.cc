/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager_condition_matcher_diagnostics_util.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

namespace {

// Neither the pref path nor the current value ever leaves the device -
// `GetValue()` only surfaces them locally on `brave://ads-internals`.
constexpr char kUnknownCurrentValue[] = "Unknown";

constexpr char kConditionMatcherMatches[] = "Yes";
constexpr char kConditionMatcherDoesNotMatch[] = "No";
constexpr char kConditionMatcherInvalid[] = "Invalid";

std::string ToConditionMatcherMatchesString(ConditionMatchResult result) {
  switch (result) {
    case ConditionMatchResult::kMatch: {
      return kConditionMatcherMatches;
    }

    case ConditionMatchResult::kNoMatch: {
      return kConditionMatcherDoesNotMatch;
    }

    case ConditionMatchResult::kInvalid: {
      return kConditionMatcherInvalid;
    }
  }

  NOTREACHED() << "Unexpected value for ConditionMatchResult: "
               << std::to_underlying(result);
}

// A numerical operator's operand can itself be a pref path rather than a
// literal number (see `MaybeResolveNumericalOperand`), so its resolved value
// is not visible from `condition` alone. Appended here for display so it does
// not have to be looked up separately.
std::string MaybeAppendResolvedNumericalOperand(
    std::string_view condition,
    const base::DictValue& virtual_prefs) {
  if (!MaybeParseNumericalOperatorType(condition)) {
    return std::string(condition);
  }

  const size_t pos = condition.find(':');
  if (pos == std::string_view::npos) {
    return std::string(condition);
  }

  double operand_as_double;
  if (base::StringToDouble(condition.substr(pos + 1), &operand_as_double)) {
    // Operand is already a literal number; nothing to resolve.
    return std::string(condition);
  }

  const std::optional<double> resolved_operand =
      MaybeResolveNumericalOperand(condition, virtual_prefs);
  if (!resolved_operand) {
    return std::string(condition);
  }

  return base::StrCat(
      {condition, " (", base::NumberToString(*resolved_operand), ")"});
}

void BuildConditionMatchersCallback(
    GetConditionMatchersDiagnosticsCallback callback,
    CreativeNewTabPageAdList creative_ads,
    base::DictValue virtual_prefs) {
  virtual_prefs.Merge(GetAdsClient().GetVirtualPrefs());

  base::ListValue list;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& [pref_path, condition] : creative_ad.condition_matchers) {
      const std::optional<std::string> current_value =
          MaybeGetPrefValueAsString(virtual_prefs, pref_path);

      const ConditionMatchResult result =
          MatchCondition(virtual_prefs, pref_path, condition);

      list.Append(
          base::DictValue()
              .Set("Creative Instance ID", creative_ad.creative_instance_id)
              .Set("Pref Path", pref_path)
              .Set("Condition", MaybeAppendResolvedNumericalOperand(
                                    condition, virtual_prefs))
              .Set("Current Value",
                   current_value.value_or(kUnknownCurrentValue))
              .Set("Matches", ToConditionMatcherMatchesString(result)));
    }
  }

  std::move(callback).Run(/*success=*/true, std::move(list));
}

}  // namespace

void GetConditionMatchersCallback(
    GetConditionMatchersDiagnosticsCallback callback,
    bool success,
    const SegmentList& /*segments*/,
    CreativeNewTabPageAdList creative_ads) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false, base::ListValue());
  }

  // Ad view/click history-dependent condition matchers (e.g.
  // `[virtual]:ad_events|<id>|<confirmation_type>`) need a batched database
  // lookup, mirroring `EligibleNewTabPageAdsV2::ApplyConditionMatcher`.
  const base::flat_set<std::string> ad_event_virtual_pref_query_ids =
      GetAdEventVirtualPrefQueryIds(creative_ads);

  database::table::AdEvents ad_events_database_table;
  ad_events_database_table.GetVirtualPrefs(
      ad_event_virtual_pref_query_ids,
      base::BindOnce(&BuildConditionMatchersCallback, std::move(callback),
                     std::move(creative_ads)));
}

void BuildTestDiagnosticsConditionMatcherResultCallback(
    TestDiagnosticsConditionMatcherCallback callback,
    std::string pref_path,
    std::string condition,
    std::optional<std::string> test_value,
    base::DictValue virtual_prefs) {
  virtual_prefs.Merge(GetAdsClient().GetVirtualPrefs());

  // Always the real resolved value, regardless of `test_value`. Shown to
  // the user so it's clear what the device's actual value is, separately
  // from whatever hypothetical value the match result below was tested
  // against.
  const std::optional<std::string> current_value =
      MaybeGetPrefValueAsString(virtual_prefs, pref_path);
  const ConditionMatchResult result = MatchCondition(
      virtual_prefs, pref_path, condition, std::move(test_value));

  std::move(callback).Run(current_value.value_or(kUnknownCurrentValue),
                          ToConditionMatcherMatchesString(result));
}

}  // namespace brave_ads

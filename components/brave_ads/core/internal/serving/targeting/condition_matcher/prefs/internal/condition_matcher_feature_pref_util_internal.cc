/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_feature_pref_util_internal.h"

#include <string_view>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_keyword_path_component_util.h"

namespace brave_ads {

namespace {

constexpr std::string_view kIsOverriddenKey = "is_overridden";
constexpr std::string_view kParamsKey = "params";

}  // namespace

std::optional<base::Value> MaybeGetFeaturePrefValue(
    std::string_view path_component) {
  std::optional<std::string_view> feature_name =
      MaybeParseKeywordPathComponentValue(path_component,
                                          kFeatureVirtualPrefKeyword);
  if (!feature_name || feature_name->empty()) {
    return std::nullopt;
  }

  base::FeatureList* const feature_list = base::FeatureList::GetInstance();

  const bool is_overridden = feature_list != nullptr &&
                             feature_list->IsFeatureOverridden(*feature_name);

  base::DictValue dict = base::DictValue().Set(
      kIsOverriddenKey, base::NumberToString(static_cast<int>(is_overridden)));

  if (feature_list) {
    const base::FieldTrial* const field_trial =
        feature_list->GetAssociatedFieldTrialByFeatureName(*feature_name);
    if (field_trial != nullptr) {
      base::FieldTrialParams field_trial_params;
      if (base::GetFieldTrialParams(field_trial->trial_name(),
                                    &field_trial_params)) {
        base::DictValue params_dict;
        for (const auto& [param_name, param_value] : field_trial_params) {
          params_dict.Set(param_name, param_value);
        }
        dict.Set(kParamsKey, std::move(params_dict));
      }
    }
  }

  return base::Value(std::move(dict));
}

}  // namespace brave_ads

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search_conversion/p3a.h"

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_search_conversion::p3a {

namespace {

constexpr char kButtonShownKey[] = "button.shown";
constexpr char kButtonTriggeredKey[] = "button.triggered";
constexpr char kNTPShownKey[] = "ntp.shown";
constexpr char kNTPTriggeredKey[] = "ntp.triggered";
constexpr char kBannerBShownKey[] = "banner_b.shown";
constexpr char kBannerBTriggeredKey[] = "banner_b.triggered";
constexpr char kBannerCShownKey[] = "banner_c.shown";
constexpr char kBannerCTriggeredKey[] = "banner_c.triggered";
constexpr char kBannerDShownKey[] = "banner_d.shown";
constexpr char kBannerDTriggeredKey[] = "banner_d.triggered";
constexpr char kDDGBannerCShownKey[] = "ddg_banner_c.shown";
constexpr char kDDGBannerCTriggeredKey[] = "ddg_banner_c.triggered";
constexpr char kDDGBannerDShownKey[] = "ddg_banner_d.shown";
constexpr char kDDGBannerDTriggeredKey[] = "ddg_banner_d.triggered";

constexpr char kSwitchSearchEngineMetric[] = "Brave.Search.SwitchEngine";

const int kMaxStoredQueryCount = 41;
const int kQueriesBeforeChurnBuckets[] = {0, 1, 2, 5, 10, 20, 40};

const char* GetPromoShownKeyName(ConversionType type) {
  switch (type) {
    case ConversionType::kBannerTypeA:
      break;
    case ConversionType::kBannerTypeB:
      return kBannerBShownKey;
    case ConversionType::kBannerTypeC:
      return kBannerCShownKey;
    case ConversionType::kBannerTypeD:
      return kBannerDShownKey;
    case ConversionType::kDDGBannerTypeC:
      return kDDGBannerCShownKey;
    case ConversionType::kDDGBannerTypeD:
      return kDDGBannerDShownKey;
    case ConversionType::kButton:
      return kButtonShownKey;
    case ConversionType::kNTP:
      return kNTPShownKey;
    case ConversionType::kNone:
      break;
  }
  NOTREACHED();
}

const char* GetPromoTriggeredKeyName(ConversionType type) {
  switch (type) {
    case ConversionType::kBannerTypeA:
      break;
    case ConversionType::kBannerTypeB:
      return kBannerBTriggeredKey;
    case ConversionType::kBannerTypeC:
      return kBannerCTriggeredKey;
    case ConversionType::kBannerTypeD:
      return kBannerDTriggeredKey;
    case ConversionType::kDDGBannerTypeC:
      return kDDGBannerCTriggeredKey;
    case ConversionType::kDDGBannerTypeD:
      return kDDGBannerDTriggeredKey;
    case ConversionType::kButton:
      // Deprecated but leave as it's used by migration code.
      return kButtonTriggeredKey;
    case ConversionType::kNTP:
      return kNTPTriggeredKey;
    case ConversionType::kNone:
      break;
  }
  NOTREACHED();
}

const char* GetPromoTypeHistogramName(ConversionType type) {
  switch (type) {
    case ConversionType::kBannerTypeA:
      break;
    case ConversionType::kBannerTypeB:
      return kSearchPromoBannerBHistogramName;
    case ConversionType::kBannerTypeC:
      return kSearchPromoBannerCHistogramName;
    case ConversionType::kBannerTypeD:
      return kSearchPromoBannerDHistogramName;
    case ConversionType::kDDGBannerTypeC:
      return kSearchPromoDDGBannerCHistogramName;
    case ConversionType::kDDGBannerTypeD:
      return kSearchPromoDDGBannerDHistogramName;
    case ConversionType::kButton:
      // Deprecated but leave as it's used by migration code.
      return kSearchPromoButtonHistogramName;
    case ConversionType::kNTP:
      return kSearchPromoNTPHistogramName;
    case ConversionType::kNone:
      break;
  }
  NOTREACHED();
}

void UpdateHistograms(PrefService* prefs) {
  // Suspend engine switch metric from
  // browser/search_engines/search_engine_tracker.cc to prevent overlap.
  UMA_HISTOGRAM_EXACT_LINEAR(kSwitchSearchEngineMetric, INT_MAX - 1, 8);

  const ConversionType types[] = {
      ConversionType::kBannerTypeB,    ConversionType::kBannerTypeC,
      ConversionType::kBannerTypeD,    ConversionType::kDDGBannerTypeC,
      ConversionType::kDDGBannerTypeD, ConversionType::kNTP};

  VLOG(1) << "SearchConversionP3A: updating histograms";

  const bool default_engine_triggered =
      prefs->GetBoolean(prefs::kP3ADefaultEngineConverted);
  const base::Value::Dict& action_statuses =
      prefs->GetDict(prefs::kP3AActionStatuses);
  for (const auto type : types) {
    const char* shown_key_name = GetPromoShownKeyName(type);
    const char* triggered_key_name = GetPromoTriggeredKeyName(type);
    const char* histogram_name = GetPromoTypeHistogramName(type);
    DCHECK(shown_key_name);
    DCHECK(triggered_key_name);
    DCHECK(histogram_name);
    if (!action_statuses.FindBoolByDottedPath(shown_key_name).value_or(false)) {
      // Do not report to P3A if promo was never shown.
      continue;
    }
    const bool promo_triggered =
        action_statuses.FindBoolByDottedPath(triggered_key_name)
            .value_or(false);

    // 0 = have not triggered promo, have not made Brave default via SERP
    // 1 = have triggered promo, have not made Brave default via SERP
    int answer = promo_triggered;

    if (default_engine_triggered) {
      // 2 = have not triggered promo, have made Brave default via SERP
      // 3 = have triggered promo, have made Brave default via SERP
      answer += 2;
    }

    base::UmaHistogramExactLinear(histogram_name, answer, 4);
  }
}

void RecordPromoAction(PrefService* prefs, const char* action_key_name) {
  const bool prev_setting = prefs->GetDict(prefs::kP3AActionStatuses)
                                .FindBoolByDottedPath(action_key_name)
                                .value_or(false);
  if (prev_setting) {
    return;
  }
  ScopedDictPrefUpdate update(prefs, prefs::kP3AActionStatuses);
  update->SetByDottedPath(action_key_name, true);
  UpdateHistograms(prefs);
}

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kP3AActionStatuses);

  registry->RegisterBooleanPref(prefs::kP3ADefaultEngineConverted, false);

  registry->RegisterIntegerPref(prefs::kP3AQueryCountBeforeChurn, 0);
  registry->RegisterBooleanPref(prefs::kP3AAlreadyChurned, false);
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 08/2023
  registry->RegisterBooleanPref(prefs::kP3ABannerShown, false);
  registry->RegisterBooleanPref(prefs::kP3ABannerTriggered, false);
  registry->RegisterBooleanPref(prefs::kP3AButtonTriggered, false);
  registry->RegisterBooleanPref(prefs::kP3ANTPTriggered, false);
  registry->RegisterBooleanPref(prefs::kP3AButtonShown, false);
  registry->RegisterBooleanPref(prefs::kP3ANTPShown, false);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 08/2023
  local_state->ClearPref(prefs::kP3ABannerShown);
  local_state->ClearPref(prefs::kP3ABannerTriggered);

  bool button_triggered = local_state->GetBoolean(prefs::kP3AButtonTriggered);
  bool button_shown = local_state->GetBoolean(prefs::kP3AButtonShown);
  if (button_triggered || button_shown) {
    ScopedDictPrefUpdate update(local_state, prefs::kP3AActionStatuses);
    update->SetByDottedPath(GetPromoShownKeyName(ConversionType::kButton),
                            button_shown);
    update->SetByDottedPath(GetPromoTriggeredKeyName(ConversionType::kButton),
                            button_triggered);
  }
  local_state->ClearPref(prefs::kP3AButtonTriggered);
  local_state->ClearPref(prefs::kP3AButtonShown);

  bool ntp_triggered = local_state->GetBoolean(prefs::kP3ANTPTriggered);
  bool ntp_shown = local_state->GetBoolean(prefs::kP3ANTPShown);
  if (ntp_triggered || ntp_shown) {
    ScopedDictPrefUpdate update(local_state, prefs::kP3AActionStatuses);
    update->SetByDottedPath(GetPromoShownKeyName(ConversionType::kNTP),
                            ntp_shown);
    update->SetByDottedPath(GetPromoTriggeredKeyName(ConversionType::kNTP),
                            ntp_triggered);
  }
  local_state->ClearPref(prefs::kP3ANTPTriggered);
  local_state->ClearPref(prefs::kP3ANTPShown);
}

void RecordPromoShown(PrefService* prefs, ConversionType type) {
  const char* key_name = GetPromoShownKeyName(type);
  DCHECK(key_name);

  VLOG(1) << "SearchConversionP3A: promo shown, key = " << key_name;

  RecordPromoAction(prefs, key_name);
}

void RecordPromoTrigger(PrefService* prefs, ConversionType type) {
  const char* key_name = GetPromoTriggeredKeyName(type);
  DCHECK(key_name);

  VLOG(1) << "SearchConversionP3A: promo triggered, key = " << key_name;

  RecordPromoAction(prefs, key_name);
}

void RecordLocationBarQuery(PrefService* prefs) {
  const int total = prefs->GetInteger(prefs::kP3AQueryCountBeforeChurn);
  if (total >= kMaxStoredQueryCount) {
    return;
  }
  prefs->SetInteger(prefs::kP3AQueryCountBeforeChurn, total + 1);
}

void RecordDefaultEngineConversion(PrefService* prefs) {
  VLOG(1) << "SearchConversionP3A: default engine converted";
  prefs->SetBoolean(prefs::kP3ADefaultEngineConverted, true);
  prefs->ClearPref(prefs::kP3AQueryCountBeforeChurn);
  UpdateHistograms(prefs);
}

void RecordDefaultEngineChurn(PrefService* prefs) {
  VLOG(1) << "SearchConversionP3A: default engine churned";
  const bool already_churned = prefs->GetBoolean(prefs::kP3AAlreadyChurned);
  const int total = prefs->GetInteger(prefs::kP3AQueryCountBeforeChurn);
  if (already_churned && total == 0) {
    // If the user already churned before, only report if they have made at
    // least one query. This will handle the case of the user switching to
    // another engine on multiple profiles.
    return;
  }
  p3a_utils::RecordToHistogramBucket(kSearchQueriesBeforeChurnHistogramName,
                                     kQueriesBeforeChurnBuckets, total);
  prefs->SetBoolean(prefs::kP3AAlreadyChurned, true);
  prefs->ClearPref(prefs::kP3AQueryCountBeforeChurn);
}

}  // namespace brave_search_conversion::p3a

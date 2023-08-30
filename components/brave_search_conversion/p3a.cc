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
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_search_conversion {
namespace p3a {

const char kSearchPromoButtonHistogramName[] = "Brave.Search.Promo.Button";
const char kSearchPromoBannerAHistogramName[] = "Brave.Search.Promo.BannerA";
const char kSearchPromoBannerBHistogramName[] = "Brave.Search.Promo.BannerB";
const char kSearchPromoBannerCHistogramName[] = "Brave.Search.Promo.BannerC";
const char kSearchPromoBannerDHistogramName[] = "Brave.Search.Promo.BannerD";
const char kSearchPromoNTPHistogramName[] = "Brave.Search.Promo.NewTabPage";

namespace {

const char kButtonShownKey[] = "button.shown";
const char kButtonTriggeredKey[] = "button.triggered";
const char kNTPShownKey[] = "ntp.shown";
const char kNTPTriggeredKey[] = "ntp.triggered";
const char kBannerAShownKey[] = "banner_a.shown";
const char kBannerATriggeredKey[] = "banner_a.triggered";
const char kBannerBShownKey[] = "banner_b.shown";
const char kBannerBTriggeredKey[] = "banner_b.triggered";
const char kBannerCShownKey[] = "banner_c.shown";
const char kBannerCTriggeredKey[] = "banner_c.triggered";
const char kBannerDShownKey[] = "banner_d.shown";
const char kBannerDTriggeredKey[] = "banner_d.triggered";

const char kSwitchSearchEngineMetric[] = "Brave.Search.SwitchEngine";

const char* GetPromoShownKeyName(ConversionType type) {
  switch (type) {
    case ConversionType::kBannerTypeA:
      return kBannerAShownKey;
    case ConversionType::kBannerTypeB:
      return kBannerBShownKey;
    case ConversionType::kBannerTypeC:
      return kBannerCShownKey;
    case ConversionType::kBannerTypeD:
      return kBannerDShownKey;
    case ConversionType::kButton:
      return kButtonShownKey;
    case ConversionType::kNTP:
      return kNTPShownKey;
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* GetPromoTriggeredKeyName(ConversionType type) {
  switch (type) {
    case ConversionType::kBannerTypeA:
      return kBannerATriggeredKey;
    case ConversionType::kBannerTypeB:
      return kBannerBTriggeredKey;
    case ConversionType::kBannerTypeC:
      return kBannerCTriggeredKey;
    case ConversionType::kBannerTypeD:
      return kBannerDTriggeredKey;
    case ConversionType::kButton:
      return kButtonTriggeredKey;
    case ConversionType::kNTP:
      return kNTPTriggeredKey;
    default:
      NOTREACHED();
      return nullptr;
  }
}

const char* GetPromoTypeHistogramName(ConversionType type) {
  switch (type) {
    case ConversionType::kBannerTypeA:
      return kSearchPromoBannerAHistogramName;
    case ConversionType::kBannerTypeB:
      return kSearchPromoBannerBHistogramName;
    case ConversionType::kBannerTypeC:
      return kSearchPromoBannerCHistogramName;
    case ConversionType::kBannerTypeD:
      return kSearchPromoBannerDHistogramName;
    case ConversionType::kButton:
      return kSearchPromoButtonHistogramName;
    case ConversionType::kNTP:
      return kSearchPromoNTPHistogramName;
    default:
      NOTREACHED();
      return nullptr;
  }
}

void UpdateHistograms(PrefService* prefs) {
  // Suspend engine switch metric from
  // browser/search_engines/search_engine_tracker.cc to prevent overlap.
  UMA_HISTOGRAM_EXACT_LINEAR(kSwitchSearchEngineMetric, INT_MAX - 1, 8);

  const ConversionType types[] = {
      ConversionType::kBannerTypeA, ConversionType::kBannerTypeB,
      ConversionType::kBannerTypeC, ConversionType::kBannerTypeD,
      ConversionType::kButton,      ConversionType::kNTP};

  VLOG(1) << "SearchConversionP3A: updating histograms";

  const bool default_engine_triggered =
      prefs->GetBoolean(prefs::kP3ADefaultEngineChanged);
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

  registry->RegisterBooleanPref(prefs::kP3ADefaultEngineChanged, false);
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

void RecordDefaultEngineChange(PrefService* prefs) {
  VLOG(1) << "SearchConversionP3A: default engine changed";
  prefs->SetBoolean(prefs::kP3ADefaultEngineChanged, true);
  UpdateHistograms(prefs);
}

}  // namespace p3a
}  // namespace brave_search_conversion

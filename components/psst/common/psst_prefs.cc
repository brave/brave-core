// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/common/psst_prefs.h"

#include <optional>

#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/psst/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace psst {

namespace {

constexpr char kEnablePsstFlag[] = "enable_psst";

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  if (base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    registry->RegisterBooleanPref(prefs::kPsstEnabled, true);
    registry->RegisterDictionaryPref(prefs::kPsstSettingsPref);
  }
}

bool GetEnablePsstFlag(PrefService* prefs) {
  if (!prefs->HasPrefPath(prefs::kPsstSettingsPref)) {
    return false;
  }

  const auto& psst_settings = prefs->GetDict(prefs::kPsstSettingsPref);
  const auto result = psst_settings.FindBool(kEnablePsstFlag);
  return result.has_value() && result.value();
}

void SetEnablePsstFlag(PrefService* prefs, const bool val) {
  ScopedDictPrefUpdate update(prefs, prefs::kPsstSettingsPref);
  base::Value::Dict& pref = update.Get();
  pref.Set(kEnablePsstFlag, val);
}

}  // namespace psst

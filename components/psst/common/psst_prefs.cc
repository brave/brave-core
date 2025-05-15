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

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  if (base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    registry->RegisterBooleanPref(prefs::kPsstEnabled, true);
    registry->RegisterDictionaryPref(prefs::kPsstSettingsPref);
  }
}

bool GetEnablePsstFlag(PrefService* prefs) {
  if (!prefs || !prefs->HasPrefPath(prefs::kPsstEnabled)) {
    return false;
  }

  return prefs->GetBoolean(prefs::kPsstEnabled);
}

void SetEnablePsstFlag(PrefService* prefs, const bool val) {
  if (!prefs) {
    return;
  }
  prefs->SetBoolean(prefs::kPsstEnabled, val);
}

}  // namespace psst

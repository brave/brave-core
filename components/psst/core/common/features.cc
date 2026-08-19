// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/common/features.h"

#include "brave/components/psst/core/browser/pref_names.h"

namespace psst::features {

BASE_FEATURE(kEnablePsst, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsPsstEnabled() {
  return base::FeatureList::IsEnabled(psst::features::kEnablePsst);
}

bool IsPsstEnabledForProfile(PrefService& pref_service) {
  return IsPsstEnabled() && pref_service.GetBoolean(prefs::kPsstEnabled);
}

}  // namespace psst::features

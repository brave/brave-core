/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/import_feature.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace settings {

BASE_FEATURE(kParallelImports,
             "ParallelImports",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Temporary flag to keep old way until
// https://github.com/brave/brave-core/pull/15637 landed.
bool IsParallelImportEnabled() {
  return base::FeatureList::IsEnabled(kParallelImports);
}

}  // namespace settings

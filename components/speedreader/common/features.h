// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_SPEEDREADER_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace speedreader {
BASE_DECLARE_FEATURE(kSpeedreaderFeature);
extern const base::FeatureParam<int> kSpeedreaderMinOutLengthParam;
}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_COMMON_FEATURES_H_

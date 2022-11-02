// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/common/features.h"

#include "brave/components/speedreader/common/buildflags.h"

namespace speedreader {

BASE_FEATURE(kSpeedreaderFeature,
             "Speedreader",
#if BUILDFLAG(ENABLE_SPEEDREADER_FEATURE)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kSpeedreaderPanelV2,
             "SpeedreaderPanelV2",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<int> kSpeedreaderMinOutLengthParam{
    &kSpeedreaderFeature, "min_out_length", 1000};

}  // namespace speedreader

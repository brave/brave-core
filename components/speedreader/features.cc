// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/features.h"

#include "base/feature_list.h"
#include "brave/components/speedreader/buildflags.h"

namespace speedreader {

const base::Feature kSpeedreaderFeature {
      "Speedreader",
#if BUILDFLAG(ENABLE_SPEEDREADER_FEATURE)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

}  // namespace speedreader

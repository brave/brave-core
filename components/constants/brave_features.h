/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_BRAVE_FEATURES_H_
#define BRAVE_COMPONENTS_CONSTANTS_BRAVE_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"

namespace features {

#if BUILDFLAG(IS_ANDROID)
extern const base::Feature kBraveRewards;
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace features

#endif  // BRAVE_COMPONENTS_CONSTANTS_BRAVE_FEATURES_H_

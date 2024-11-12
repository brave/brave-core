// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat/core/common/features.h"

#include "base/feature_list.h"

namespace webcompat::features {

// This flag enables the webcompat exceptions service, which allows
// a remote list to control site-specific exceptions to Brave features
// when required for web compatibility.
BASE_FEATURE(kBraveWebcompatExceptionsService,
             "BraveWebcompatExceptionsService",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace webcompat::features

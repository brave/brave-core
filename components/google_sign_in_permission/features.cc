// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/google_sign_in_permission/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace google_sign_in_permission::features {

// When enabled, Brave will prompt for permission on sites which want to use
// Google Sign In.
BASE_FEATURE(kBraveGoogleSignInPermission,
             "BraveGoogleSignInPermission",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace google_sign_in_permission::features

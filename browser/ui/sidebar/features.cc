// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/sidebar/features.h"

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"
#include "build/buildflag.h"

namespace sidebar::features {

// kSidebarV2 is enabled by default when ENABLE_SIDEBAR_V2 buildflag is on,
// since that build uses upstream's SidePanel and requires V2 behavior.
BASE_FEATURE(kSidebarV2,
#if BUILDFLAG(ENABLE_SIDEBAR_V2)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);

}  // namespace sidebar::features

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/features.h"

#include "base/feature_list.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"

namespace brave_sync {
namespace features {

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
const base::Feature kBraveSync{"BraveSync", base::FEATURE_ENABLED_BY_DEFAULT};
#else
const base::Feature kBraveSync{"BraveSync", base::FEATURE_DISABLED_BY_DEFAULT};
#endif

}  // namespace features
}  // namespace brave_sync

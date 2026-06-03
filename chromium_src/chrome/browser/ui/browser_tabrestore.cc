// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_tabrestore.h"

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/tab_restore_utils.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/browser_tabrestore.cc>

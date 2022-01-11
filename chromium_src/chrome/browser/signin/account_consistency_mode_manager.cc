/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/signin/account_consistency_mode_manager.h"

#include "base/notreached.h"

#if defined(OS_ANDROID)
#undef BUILDFLAG_INTERNAL_ENABLE_MIRROR
#define BUILDFLAG_INTERNAL_ENABLE_MIRROR() (0)

#undef NOTREACHED
#define NOTREACHED() DCHECK(true)
#endif

#include "src/chrome/browser/signin/account_consistency_mode_manager.cc"

#if defined(OS_ANDROID)
#undef BUILDFLAG_INTERNAL_ENABLE_MIRROR
#undef NOTREACHED
#endif

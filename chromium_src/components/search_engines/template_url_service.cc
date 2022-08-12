/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
// Forward include these headers to:
// 1. Avoid potential re-defining NO_REGISTRATION_FLAGS in other includes;
// 2. Perform static_asserts below to ensure the trick with define works as we
// expect.
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry.h"

static_assert(PrefRegistry::NO_REGISTRATION_FLAGS == 0);
static_assert(user_prefs::PrefRegistrySyncable::SYNCABLE_PREF == 1);

#define NO_REGISTRATION_FLAGS \
  NO_REGISTRATION_FLAGS + user_prefs::PrefRegistrySyncable::SYNCABLE_PREF
#endif  // BUILDFLAG(IS_ANDROID)

#include "src/components/search_engines/template_url_service.cc"

#if BUILDFLAG(IS_ANDROID)
#undef NO_REGISTRATION_FLAGS
#endif

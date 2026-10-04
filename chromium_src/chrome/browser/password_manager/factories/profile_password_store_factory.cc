/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#define BRAVE_PROFILE_PASSWORD_STORE_FACTORY_BUILD_PASSWORD_STORE \
  affiliation_service = AffiliationServiceFactory::GetForProfile(profile);
#else
#define BRAVE_PROFILE_PASSWORD_STORE_FACTORY_BUILD_PASSWORD_STORE
#endif

#include <chrome/browser/password_manager/factories/profile_password_store_factory.cc>

#undef BRAVE_PROFILE_PASSWORD_STORE_FACTORY_BUILD_PASSWORD_STORE

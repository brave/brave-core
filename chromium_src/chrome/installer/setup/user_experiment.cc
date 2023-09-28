/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/install_static/buildflags.h"

#if defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_stash_google_update_integration.h"
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (1)
#endif

#include "src/chrome/installer/setup/user_experiment.cc"

#if defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_restore_google_update_integration.h"
#endif

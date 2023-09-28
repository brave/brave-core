/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#include "chrome/install_static/buildflags.h"
#include "chrome/install_static/install_constants.h"

#if defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_stash_google_update_integration.h"
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (0)
#endif

#include "src/chrome/browser/metrics/google_update_metrics_provider_win.cc"

#if defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_restore_google_update_integration.h"
#endif

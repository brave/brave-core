/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "chrome/install_static/buildflags.h"

#if BUILDFLAG(IS_WIN) && defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_enable_google_update_integration.h"
#endif

#include "src/chrome/browser/active_use_util.cc"

#if BUILDFLAG(IS_WIN) && defined(OFFICIAL_BUILD)
#include "chrome/install_static/brave_restore_google_update_integration.h"
#endif

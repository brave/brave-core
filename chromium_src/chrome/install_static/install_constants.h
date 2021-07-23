/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific types and constants for Google Chrome.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_CONSTANTS_H_

#if defined(OFFICIAL_BUILD)
#include "chrome/install_static/buildflags.h"
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
#define BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION() (1)
#endif

#include "../../../../chrome/install_static/install_constants.h"

#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_USE_GOOGLE_UPDATE_INTEGRATION
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_CONSTANTS_H_

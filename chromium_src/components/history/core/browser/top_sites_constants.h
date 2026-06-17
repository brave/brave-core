/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_CONSTANTS_H_

#include "build/build_config.h"

// This chromium_src override is required only to include ^ build_config.h to
// be used at top_sites_constants.h.yaml plaster for #if BUILDFLAG(IS_ANDROID)

#include <components/history/core/browser/top_sites_constants.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_TOP_SITES_CONSTANTS_H_

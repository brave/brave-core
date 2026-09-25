// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_GRIT_CHROME_UNSCALED_RESOURCES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_GRIT_CHROME_UNSCALED_RESOURCES_H_

#include "build/build_config.h"

// Brave's unscaled resources are desktop-only.
#if !BUILDFLAG(IS_ANDROID)
#include "brave/grit/brave_unscaled_resources.h"  // IWYU pragma: export
#endif                                            // !BUILDFLAG(IS_ANDROID)

#include "../gen/chrome/grit/chrome_unscaled_resources.h"  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_GRIT_CHROME_UNSCALED_RESOURCES_H_

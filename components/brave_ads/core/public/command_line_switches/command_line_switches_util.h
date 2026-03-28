/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_COMMAND_LINE_SWITCHES_COMMAND_LINE_SWITCHES_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_COMMAND_LINE_SWITCHES_COMMAND_LINE_SWITCHES_UTIL_H_

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

// Builds CommandLineSwitches based on command line arguments and environment.
mojom::CommandLineSwitchesPtr BuildCommandLineSwitches();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_COMMAND_LINE_SWITCHES_COMMAND_LINE_SWITCHES_UTIL_H_

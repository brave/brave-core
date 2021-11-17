/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_utils.h"

#include "base/command_line.h"
#include "brave/components/skus/browser/switches.h"

namespace skus {

std::string GetEnvironment() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kSkusEnv)) {
#if defined(OFFICIAL_BUILD)
    return kEnvProduction;
#else
    return kEnvDevelopment;
#endif
  }

  const std::string value = cmd->GetSwitchValueASCII(switches::kSkusEnv);
  DCHECK(value == kEnvProduction || value == kEnvStaging ||
         value == kEnvDevelopment);
  return value;
}

}  // namespace skus

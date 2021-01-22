// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/common/urls.h"

#include <string>

#include "base/command_line.h"
#include "brave/components/brave_today/common/switches.h"

namespace brave_today {

std::string GetHostname() {
  std::string from_switch =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kBraveTodayHost);
  if (from_switch.empty()) {
    return "brave-today-cdn.brave.com";
  } else {
    return from_switch;
  }
}

}  // namespace brave_today

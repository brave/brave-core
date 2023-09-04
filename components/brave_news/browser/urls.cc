// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/urls.h"

#include <string>

#include "base/command_line.h"
#include "brave/components/brave_news/common/switches.h"

namespace brave_news {

std::string GetHostname() {
  std::string from_switch =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kBraveNewsHost);
  if (from_switch.empty()) {
    return "brave-today-cdn.brave.com";
  } else {
    return from_switch;
  }
}

}  // namespace brave_news

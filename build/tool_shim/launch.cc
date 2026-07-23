/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/tool_shim/launch.h"

#include <ostream>

#include "base/strings/string_util.h"

std::ostream& operator<<(std::ostream& os, const LaunchConfig& config) {
  os << "Launch config:"
     << "\ncmd: " << base::JoinString(config.argv, FILE_PATH_LITERAL(" "))
     << "\ncwd: " << config.cwd;
  if (!config.env.empty()) {
    os << "\nenv:";
    for (const auto& [key, val] : config.env) {
      os << "\n  " << key << "=" << val;
    }
  }
  return os;
}

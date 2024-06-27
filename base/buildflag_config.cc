/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/buildflag_config.h"

#include "base/containers/flat_map.h"
#include "build/buildflag.h"

namespace brave {

namespace {
base::flat_map<std::string, std::string> g_build_flag_config_override_map;
}  // namespace

BuildflagConfig::BuildflagConfig(std::string_view name, std::string_view value)
    : name_(name), value_(value) {}

std::string BuildflagConfig::Get() const {
  auto it = g_build_flag_config_override_map.find(name_);
  if (it != g_build_flag_config_override_map.end()) {
    return it->second;
  }
  return value_;
}

ScopedBuildflagConfigOverride::ScopedBuildflagConfigOverride(
    std::string_view name,
    std::string_view value)
    : BuildflagConfig(name, value) {
  g_build_flag_config_override_map[name_] = value;
}

ScopedBuildflagConfigOverride::~ScopedBuildflagConfigOverride() {
  g_build_flag_config_override_map.erase(name_);
}

// always return the value that was set in the constructor
std::string ScopedBuildflagConfigOverride::Get() const {
  return value_;
}

}  // namespace brave

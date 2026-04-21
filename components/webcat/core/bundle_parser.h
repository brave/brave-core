/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCAT_CORE_BUNDLE_PARSER_H_
#define BRAVE_COMPONENTS_WEBCAT_CORE_BUNDLE_PARSER_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/webcat/core/constants.h"

namespace webcat {

struct Manifest {
  std::string app;
  std::string version;
  std::string default_csp;
  std::map<std::string, std::string> extra_csp;
  std::string default_index;
  std::string default_fallback;
  std::map<std::string, std::string> files;
  std::vector<std::string> wasm;
};

struct Bundle {
  Manifest manifest;
  std::map<std::string, std::string> signatures;
};

struct ParseResult {
  WebcatError error = WebcatError::kNone;
  std::string error_detail;
  std::optional<Bundle> bundle;
};

ParseResult ParseBundle(const std::string& json_string);

bool ValidateManifestStructure(const Manifest& manifest, std::string& error_detail);

}  // namespace webcat

#endif  // BRAVE_COMPONENTS_WEBCAT_CORE_BUNDLE_PARSER_H_

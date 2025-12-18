/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/search_query_metrics_environment_util.h"

#include <string>
#include <string_view>

#include "base/command_line.h"
#include "base/strings/string_tokenizer.h"

namespace metrics {

namespace {
constexpr std::string_view kSearchQueryMetricsSwitch = "search-query-metrics";
}  // namespace

bool ShouldUseStagingEnvironment() {
  const auto* const command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(kSearchQueryMetricsSwitch)) {
    return false;
  }

  const std::string value = base::ToLowerASCII(
      command_line->GetSwitchValueASCII(kSearchQueryMetricsSwitch));
  return value == "staging" || value == "staging=true" || value == "staging=1";
}

}  // namespace metrics

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_LEGACY_REFINE_FUNCTIONS_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_LEGACY_REFINE_FUNCTIONS_H_

#include <optional>
#include <string>

#include "brave/components/web_discovery/browser/patterns.h"

namespace web_discovery {

// Executes legacy refine functions for v1 patterns
std::optional<std::string> ExecuteRefineFunctions(
    const RefineFunctionList& function_list,
    std::string value);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_LEGACY_REFINE_FUNCTIONS_H_

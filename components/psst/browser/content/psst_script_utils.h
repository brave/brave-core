// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPT_UTILS_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPT_UTILS_H_

#include <optional>
#include <string>

#include "base/values.h"

namespace psst {

std::string GetScriptWithParams(const std::string& script,
                                std::optional<base::Value> params);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPT_UTILS_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_HELPER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_shields {

void MergeCspDirectiveInto(absl::optional<std::string> from,
                           absl::optional<std::string>* into);

void MergeResourcesInto(base::Value::Dict from,
                        base::Value::Dict* into,
                        bool force_hide);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_HELPER_H_

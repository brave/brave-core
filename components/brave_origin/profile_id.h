/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_PROFILE_ID_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_PROFILE_ID_H_

#include <string>

#include "base/files/file_path.h"

namespace brave_origin {

// Creates a base64url encoded profile identifier for stable, safe identifier
std::string GetProfileId(const base::FilePath& profile_path);

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_PROFILE_ID_H_

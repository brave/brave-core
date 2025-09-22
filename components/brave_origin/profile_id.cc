/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/profile_id.h"

#include "base/base64url.h"

namespace brave_origin {

std::string GetProfileId(const base::FilePath& profile_path) {
  std::string profile_path_base_name = profile_path.BaseName().AsUTF8Unsafe();
  if (profile_path_base_name.empty()) {
    profile_path_base_name = "Default";
  }
  std::string profile_id;
  base::Base64UrlEncode(profile_path_base_name,
                        base::Base64UrlEncodePolicy::OMIT_PADDING, &profile_id);
  return profile_id;
}

}  // namespace brave_origin

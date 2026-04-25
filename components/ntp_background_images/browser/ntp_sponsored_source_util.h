/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_SOURCE_UTIL_H_
#define BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_SOURCE_UTIL_H_

#include <optional>
#include <string>
#include <vector>

namespace base {
class FilePath;
}  // namespace base

namespace ntp_background_images {

struct Campaign;

std::optional<std::string> ReadFileToString(const base::FilePath& path);

// Sandbox the request to the campaign creative directory or its children to
// prevent path traversal.
std::optional<base::FilePath> MaybeGetFilePathForRequestPath(
    const base::FilePath& request_path,
    const std::vector<Campaign>& campaigns);

}  // namespace ntp_background_images

#endif  // BRAVE_COMPONENTS_NTP_BACKGROUND_IMAGES_BROWSER_NTP_SPONSORED_SOURCE_UTIL_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_util.h"

#include "base/check.h"
#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

namespace ntp_background_images {

namespace {

std::optional<base::FilePath> MaybeGetChildCreativeSubdirectories(
    const base::FilePath& parent_creative_dir,
    const base::FilePath& request_dir) {
  base::FilePath relative_path;
  if (!parent_creative_dir.AppendRelativePath(request_dir, &relative_path)) {
    // Path traversal, deny access.
    return std::nullopt;
  }

  return relative_path;
}

}  // namespace

std::optional<base::FilePath> MaybeGetFilePathForRequestPath(
    const base::FilePath& request_file_path,
    const std::vector<Campaign>& campaigns) {
  if (request_file_path.ReferencesParent()) {
    // Path traversal, deny access.
    return std::nullopt;
  }

  const base::FilePath request_dir = request_file_path.DirName();
  const base::FilePath request_file = request_file_path.BaseName();

  for (const auto& campaign : campaigns) {
    for (const auto& creative : campaign.creatives) {
      CHECK(!creative.file_path.ReferencesParent());

      base::FilePath creative_dir = creative.file_path.DirName();
      const base::FilePath parent_creative_dir = creative_dir.BaseName();

      const bool is_within_allowed_dir =
          parent_creative_dir == request_dir ||
          parent_creative_dir.IsParent(request_dir);
      if (!is_within_allowed_dir) {
        // The creative parent directory did not match the request directory and
        // is not a child of the request directory.
        continue;
      }

      if (const std::optional<base::FilePath> child_creative_subdirectories =
              MaybeGetChildCreativeSubdirectories(parent_creative_dir,
                                                  request_dir)) {
        creative_dir = creative_dir.Append(*child_creative_subdirectories);
      }

      creative_dir = creative_dir.Append(request_file);

      return creative_dir.NormalizePathSeparators();
    }
  }

  // Path traversal, deny access.
  return std::nullopt;
}

}  // namespace ntp_background_images

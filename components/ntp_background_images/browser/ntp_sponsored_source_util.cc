/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_util.h"

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

namespace ntp_background_images {

namespace {

// Determines whether the `request_dir` is either the same as the
// `parent_creative_dir` or a subdirectory of it. This validation ensures that
// the requested directory is confined within the allowed directory hierarchy,
// preventing unauthorized access to directories outside the intended scope.
bool IsPathWithinParentDir(const base::FilePath& parent_creative_dir,
                           const base::FilePath& request_dir) {
  return parent_creative_dir == request_dir ||
         parent_creative_dir.IsParent(request_dir);
}

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

std::optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::nullopt;
  }
  return contents;
}

std::optional<base::FilePath> MaybeGetFilePathForRequestPath(
    const base::FilePath& request_path,
    const std::vector<Campaign>& campaigns) {
  if (request_path.ReferencesParent()) {
    // Path traversal, deny access.
    return std::nullopt;
  }

  const base::FilePath request_dir = request_path.DirName();
  const base::FilePath request_file = request_path.BaseName();

  for (const auto& campaign : campaigns) {
    for (const auto& creative : campaign.creatives) {
      CHECK(!creative.file_path.ReferencesParent());

      base::FilePath creative_dir = creative.file_path.DirName();
      const base::FilePath parent_creative_dir = creative_dir.BaseName();

      if (!IsPathWithinParentDir(parent_creative_dir, request_dir)) {
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

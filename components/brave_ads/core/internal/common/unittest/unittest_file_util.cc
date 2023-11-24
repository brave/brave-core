/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"

#include "base/files/file_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_tag_parser_util.h"

namespace brave_ads {

namespace {

absl::optional<std::string> MaybeReadFileToString(const base::FilePath& path) {
  std::string content;
  if (!base::ReadFileToString(path, &content)) {
    return absl::nullopt;
  }

  return content;
}

}  // namespace

absl::optional<std::string> MaybeReadFileResourceToString(
    const std::string& name) {
  const base::FilePath path = TestDataFileResourcesPath().AppendASCII(name);
  return MaybeReadFileToString(path);
}

absl::optional<std::string> MaybeReadAndReplaceTagsForFileResourceToString(
    const std::string& name) {
  absl::optional<std::string> contents = MaybeReadFileResourceToString(name);
  if (!contents) {
    return absl::nullopt;
  }

  ParseAndReplaceTags(*contents);

  return *contents;
}

absl::optional<std::string> MaybeReadDataResourceToString(
    const std::string& name) {
  const base::FilePath path = DataResourcesPath().AppendASCII(name);
  return MaybeReadFileToString(path);
}

}  // namespace brave_ads

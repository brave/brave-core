/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/file_test_util.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/tag_parser_test_util_internal.h"

namespace brave_ads::test {

std::optional<std::string> MaybeReadFileToString(const std::string& name) {
  const base::FilePath path = DataPath().AppendASCII(name);

  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::nullopt;
  }

  return contents;
}

std::optional<std::string> MaybeReadFileToStringAndReplaceTags(
    const std::string& name) {
  std::optional<std::string> contents = MaybeReadFileToString(name);
  if (!contents) {
    return std::nullopt;
  }

  ParseAndReplaceTags(*contents);

  return contents;
}

std::optional<std::string> MaybeReadDataResourceToString(
    const std::string& name) {
  const base::FilePath path = DataResourcesPath().AppendASCII(name);

  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::nullopt;
  }

  return contents;
}

}  // namespace brave_ads::test

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_file_util.h"

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "bat/ads/internal/unittest_tag_parser_util.h"

namespace ads {

namespace {

base::FilePath GetDataPath() {
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("brave");
  path = path.AppendASCII("vendor");
  path = path.AppendASCII("bat-native-ads");
  path = path.AppendASCII("data");
  return path;
}

}  // namespace

base::FilePath GetTestPath() {
  base::FilePath path = GetDataPath();
  path = path.AppendASCII("test");
  return path;
}

absl::optional<std::string> ReadFileFromTestPathToString(
    const std::string& name) {
  base::FilePath path = GetTestPath();
  path = path.AppendASCII(name);

  std::string value;
  if (!base::ReadFileToString(path, &value)) {
    return absl::nullopt;
  }

  ParseAndReplaceTagsForText(&value);

  return value;
}

base::FilePath GetResourcesPath() {
  base::FilePath path = GetDataPath();
  path = path.AppendASCII("resources");
  return path;
}

absl::optional<std::string> ReadFileFromResourcePathToString(
    const std::string& name) {
  base::FilePath path = GetResourcesPath();
  path = path.AppendASCII(name);

  std::string value;
  if (!base::ReadFileToString(path, &value)) {
    return absl::nullopt;
  }

  return value;
}

}  // namespace ads

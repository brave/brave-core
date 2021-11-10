/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_DAT_FILE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_DAT_FILE_UTIL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"

namespace brave_component_updater {

using DATFileDataBuffer = std::vector<unsigned char>;

void GetDATFileData(const base::FilePath& file_path, DATFileDataBuffer* buffer);
std::string GetDATFileAsString(const base::FilePath& file_path);

DATFileDataBuffer ReadDATFileData(const base::FilePath& dat_file_path);

template <typename T>
using LoadDATFileDataResult =
    std::pair<std::unique_ptr<T>, brave_component_updater::DATFileDataBuffer>;

template <typename T>
LoadDATFileDataResult<T> LoadDATFileData(const base::FilePath& dat_file_path) {
  DATFileDataBuffer buffer = ReadDATFileData(dat_file_path);
  std::unique_ptr<T> client;
  client = std::make_unique<T>();
  if (buffer.empty() ||
      !client->deserialize(reinterpret_cast<char*>(&buffer.front()),
                           buffer.size()))
    client.reset();
  return LoadDATFileDataResult<T>(std::move(client), std::move(buffer));
}

template <typename T>
LoadDATFileDataResult<T> LoadRawFileData(const base::FilePath& dat_file_path) {
  DATFileDataBuffer buffer = ReadDATFileData(dat_file_path);
  std::unique_ptr<T> client;

  if (!buffer.empty())
    client = std::make_unique<T>(reinterpret_cast<char*>(&buffer.front()),
                                 buffer.size());

  return LoadDATFileDataResult<T>(std::move(client), std::move(buffer));
}

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_DAT_FILE_UTIL_H_

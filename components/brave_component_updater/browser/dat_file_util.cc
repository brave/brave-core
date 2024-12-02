/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/dat_file_util.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"

namespace {

void GetDATFileData(const base::FilePath& file_path,
                    brave_component_updater::DATFileDataBuffer* buffer) {
  if (!base::PathExists(file_path)) {
    LOG(ERROR) << "GetDATFileData: the dat file is not found. " << file_path;
    return;
  }

  if (auto bytes = base::ReadFileToBytes(file_path)) {
    *buffer = std::move(*bytes);
  } else {
    LOG(ERROR) << "GetDATFileData: cannot "
               << "read dat file " << file_path;
  }
}

}  // namespace

namespace brave_component_updater {

DATFileDataBuffer ReadDATFileData(const base::FilePath& dat_file_path) {
  TRACE_EVENT_BEGIN1("brave.adblock", "ReadDATFileData", "path",
                     dat_file_path.MaybeAsASCII());
  DATFileDataBuffer buffer;
  GetDATFileData(dat_file_path, &buffer);
  TRACE_EVENT_END1("brave.adblock", "ReadDATFileData", "size", buffer.size());
  return buffer;
}

std::string GetDATFileAsString(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    LOG(ERROR) << "GetDATFileAsString: cannot "
               << "read dat file " << file_path;
  }
  return contents;
}

}  // namespace brave_component_updater

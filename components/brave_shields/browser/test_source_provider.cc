/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/test_source_provider.h"

#include <string>
#include <utility>

namespace brave_shields {

TestSourceProvider::TestSourceProvider(const std::string& rules,
                                       const std::string& resources)
    : rules_(rules), resources_(resources) {}

TestSourceProvider::TestSourceProvider(const base::FilePath& dat_location,
                                       const std::string& resources)
    : resources_(resources) {
  CHECK(!dat_location.empty());

  dat_buffer_ = brave_component_updater::ReadDATFileData(dat_location);

  CHECK(!dat_buffer_.empty());
}

TestSourceProvider::~TestSourceProvider() {}

void TestSourceProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  if (dat_buffer_.empty()) {
    auto buffer = std::vector<unsigned char>(rules_.begin(), rules_.end());
    std::move(cb).Run(false, buffer);
  } else {
    std::move(cb).Run(true, dat_buffer_);
  }
}

void TestSourceProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> cb) {
  std::move(cb).Run(resources_);
}

}  // namespace brave_shields

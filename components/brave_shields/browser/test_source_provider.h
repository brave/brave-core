/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TEST_SOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TEST_SOURCE_PROVIDER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/browser/ad_block_source_provider.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

class TestSourceProvider : public AdBlockSourceProvider,
                           public AdBlockResourceProvider {
 public:
  TestSourceProvider(const std::string& rules, const std::string& resources);
  TestSourceProvider(const base::FilePath& dat_location,
                     const std::string& resources);
  ~TestSourceProvider() override;

  void LoadDATBuffer(
      base::OnceCallback<void(bool deserialize,
                              const DATFileDataBuffer& dat_buf)> cb) override;

  void LoadResources(
      base::OnceCallback<void(const std::string& resources_json)> cb) override;

 private:
  DATFileDataBuffer dat_buffer_;
  std::string rules_;
  std::string resources_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TEST_SOURCE_PROVIDER_H_

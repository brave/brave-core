/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/cdm_registry.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/token.h"
#include "content/public/common/cdm_info.h"
#include "content/test/test_content_client.h"
#include "media/base/decrypt_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class TestClient : public TestContentClient {
 public:
  TestClient() {}
  ~TestClient() override {}

  void AddContentDecryptionModules(
      std::vector<content::CdmInfo>* cdms,
      std::vector<media::CdmHostFilePath>* cdm_host_file_paths) override {
    CdmCapability capability;
    capability.encryption_schemes.insert(media::EncryptionMode::kCenc);
    cdms->push_back(
        content::CdmInfo(std::string(), base::Token(), base::Version(),
                         base::FilePath(), std::string(),
                         capability, "com.widevine.alpha", false));
  }
};

}  // namespace

class CdmRegistryImplTest : public testing::Test {
 public:
  CdmRegistryImplTest() {}
  ~CdmRegistryImplTest() override {}

 private:
  void SetUp() override {
    SetContentClient(&client_);
  }

  TestClient client_;
};

// This test checks CdmRegistryImpl erases widevine from cdm list after fetching
// cdm list from content client. This erasure happens only on linux.
TEST_F(CdmRegistryImplTest, WidevineCdmExcludeTest) {
  auto* cdm_registry = CdmRegistry::GetInstance();
  cdm_registry->Init();

  const auto& cdms = cdm_registry->GetAllRegisteredCdms();
#if defined(OS_LINUX)
  EXPECT_EQ(0U, cdms.size());
#else
  EXPECT_EQ(1U, cdms.size());
#endif
}

}  // namespace content

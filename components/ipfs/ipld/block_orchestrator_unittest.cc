/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_orchestrator.h"

#include <cstdint>
#include <memory>
#include <thread>

#include "base/containers/contains.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/test_timeouts.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/ipld/block.h"
#include "brave/components/ipfs/ipld/car_block_reader.h"
#include "brave/components/ipfs/ipld/car_content_requester.h"
#include "brave/components/ipfs/ipld/content_requester.h"
#include "brave/components/ipfs/ipld/trustless_client_types.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_host_resolver.h"
#include "gtest/gtest.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/test/fake_test_cert_verifier_params_factory.h"
#include "services/network/test/test_network_context_client.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/unrar/src/rartypes.hpp"
#include "url/gurl.h"

namespace {
constexpr char kSubDirWithMixedBlockFiles[] =
    "subdir-with-mixed-block-files.car";
constexpr char kTestDataSubDir[] = "ipfs/ipld";

const struct {
  std::string ipfs_url;
  std::string file_content;
  uint64_t size;
} kOneFileExtractInputData[] = {
    {"ipfs://bafybeigcisqd7m5nf3qmuvjdbakl5bdnh4ocrmacaqkpuh77qjvggmt2sa",
     "subdir_multiblock.txt", 1026},
    {"ipfs://bafkreifjjcie6lypi6ny7amxnfftagclbuxndqonfipmb64f2km2devei4",
     "subdir_hello.txt", 12}};

bool CompareVeAndStr(const std::vector<uint8_t>& vec, const std::string& str) {
  if (vec.size() != str.size()) {
    return false;
  }
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i] != str[i]) {
      return false;
    }
  }
  return true;
}

}  // namespace

namespace ipfs::ipld {

class BlockOrchestratorUnitTest : public testing::Test {
 public:
  BlockOrchestratorUnitTest() {
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    test_data_dir_ = test_data_dir_.AppendASCII(kTestDataSubDir);
  }
  ~BlockOrchestratorUnitTest() override = default;

 protected:
  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  network::TestURLLoaderFactory* url_loader_factory() {
    return &url_loader_factory_;
  }

  base::test::TaskEnvironment* task_environment() { return &task_environment_; }
  const base::FilePath& test_data_dir() const { return test_data_dir_; }

  std::string GetFileContent(const std::string& filename) const {
    std::string result;

    const auto full_path = test_data_dir_.AppendASCII(filename);
    /* void() for assert */
    [&]() { ASSERT_TRUE(base::ReadFileToString(full_path, &result)); }();
    return result;
  }

  void TestGetCarFileByIpfsCid(const std::string& ipfs_url,
                               const std::string& car_file_name, const uint64_t& size) {
    GURL url(ipfs_url);
    auto req = std::make_unique<IpfsTrustlessRequest>(
        url, base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                 url_loader_factory()));
    auto orchestrator = std::make_unique<BlockOrchestrator>(GetPrefs());
    url_loader_factory()->SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          ASSERT_TRUE(request.url.is_valid());
          ASSERT_TRUE(ipfs::IsDefaultGatewayURL(request.url, GetPrefs()));

          ASSERT_TRUE(base::Contains(request.url.query(), "format=car"));
          ASSERT_TRUE(base::Contains(request.url.query(), "dag-scope=entity"));
          ASSERT_TRUE(base::Contains(
              net::UnescapePercentEncodedUrl(request.url.query()),
              "entity-bytes=0:0"));

          auto response_head = network::mojom::URLResponseHead::New();
          response_head->headers =
              base::MakeRefCounted<net::HttpResponseHeaders>("");

          response_head->headers->SetHeader(
              "Content-Type",
              "application/vnd.ipld.car; version=1; order=dfs; dups=n");
          response_head->headers->ReplaceStatusLine("HTTP/1.1 200 OK");
          auto content = GetFileContent(kSubDirWithMixedBlockFiles);
          url_loader_factory()->AddResponse(
              request.url, std::move(response_head), content,
              network::URLLoaderCompletionStatus(net::OK));
        }));

    std::vector<uint8_t> received_data;
    int last_chunk_received_counter{0};
    auto orchestrator_request_callback = base::BindLambdaForTesting(
        [&](std::unique_ptr<IpfsTrustlessRequest> request,
            std::unique_ptr<IpfsTrustlessResponse> response) {
          EXPECT_TRUE(response);
          received_data.insert(received_data.end(), response->body.begin(),
                               response->body.end());
          if(response->is_last_chunk){
            last_chunk_received_counter++;
          }
          LOG(INFO) << "[IPFS] total_size: " << response->total_size;
          EXPECT_EQ(response->total_size, size);
        });
    orchestrator->BuildResponse(std::move(req),
                                std::move(orchestrator_request_callback));
    ASSERT_TRUE(orchestrator->IsActive());
    task_environment()->RunUntilIdle();
    EXPECT_TRUE(CompareVeAndStr(received_data, GetFileContent(car_file_name)));
    EXPECT_EQ(last_chunk_received_counter, 1);

    orchestrator->Reset();
    ASSERT_FALSE(orchestrator->IsActive());
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::FilePath test_data_dir_;
  std::unique_ptr<TestingProfile> profile_;
  network::TestURLLoaderFactory url_loader_factory_;
};

TEST_F(BlockOrchestratorUnitTest, RequestFile) {
  base::ranges::for_each(
      kOneFileExtractInputData, [this](const auto& item_case) {
        TestGetCarFileByIpfsCid(item_case.ipfs_url, item_case.file_content, item_case.size);
      });
}

TEST_F(BlockOrchestratorUnitTest, RequestMultiblockFile) {
        TestGetCarFileByIpfsCid("ipfs://bafybeigcisqd7m5nf3qmuvjdbakl5bdnh4ocrmacaqkpuh77qjvggmt2sa", "subdir_multiblock.txt", 1026);
  
}

}  // namespace ipfs::ipld

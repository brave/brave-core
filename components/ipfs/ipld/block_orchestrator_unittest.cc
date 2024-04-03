/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_orchestrator.h"

#include <cstdint>
#include <memory>
#include <regex>
#include <thread>

#include "absl/types/optional.h"
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
#include "brave/components/ipfs/ipfs_constants.h"
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
constexpr char kSubDirWithSingleBlockFiles[] =
    "subdir-with-single-block-files.car";
constexpr char kFolderWithIndexHtmlFiles[] = "folder_with_index_html.car";
constexpr char kJustFolderWithNoIndexHtmlFiles[] =
    "folder_with_index_html_just_folder.car";

constexpr char kTestDataSubDir[] = "ipfs/ipld";

struct CarFileTestData {
  std::string car_file_name;
  std::string file_content;
  uint64_t size;  
};

const struct {
  std::string test_name;
  std::string ipfs_url;
  std::map<std::string, CarFileTestData> cids_to_car_map;
} kOneFileExtractInputData[] = {
    {"MultiBlockFileTest",
     "ipfs://bafybeigcisqd7m5nf3qmuvjdbakl5bdnh4ocrmacaqkpuh77qjvggmt2sa",
     {{"bafybeigcisqd7m5nf3qmuvjdbakl5bdnh4ocrmacaqkpuh77qjvggmt2sa",
       {kSubDirWithMixedBlockFiles, "subdir_multiblock.txt", 1026}}}},
    {"SingleBlockFileTest",
     "ipfs://bafkreifjjcie6lypi6ny7amxnfftagclbuxndqonfipmb64f2km2devei4",
     {{"bafkreifjjcie6lypi6ny7amxnfftagclbuxndqonfipmb64f2km2devei4",
       {kSubDirWithSingleBlockFiles, "subdir_hello.txt", 12}}}},
    {"RequestIndexFileInTheFolderTest",
     "ipfs://bafybeidtkposquyc4h6lznimdqay6vf3tcrcyso2s4lqb5j2os3z7ebxjm",
     {{"bafybeidtkposquyc4h6lznimdqay6vf3tcrcyso2s4lqb5j2os3z7ebxjm",
       {kJustFolderWithNoIndexHtmlFiles, "folder_with_index_html_index.html",
        362}},
      {"bafkreibfdmgv63epr2cmhhpbtvrwc4hhc4mjlpbigep3lj5tiexdgnukeq",
       {kFolderWithIndexHtmlFiles, "folder_with_index_html_index.html",
        362}}}}};

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

absl::optional<std::string> GetCidFromUrl(const std::string url) {
  std::regex pattern("\\/ip.+s\\/([a-z0-9]+)");
  if (std::smatch match; std::regex_search(url, match, pattern)) {
    return match[1];
  }
  LOG(INFO) << "[IPFS] GetCidFromUrl FAILED url:" << url;
  return absl::nullopt;
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

  void TestGetCarFileByIpfsCid(
      const std::string& test_name,
      const std::string& ipfs_url,
      const std::map<std::string, CarFileTestData>& cids_to_car_map) {
    GURL url(ipfs_url);
    auto req = std::make_unique<IpfsTrustlessRequest>(
        url,
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            url_loader_factory()),
        false);
    auto orchestrator = std::make_unique<BlockOrchestrator>(GetPrefs());
    const CarFileTestData* current_cid_file_test_data_ptr = nullptr;
    url_loader_factory()->SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory()->ClearResponses();
          ASSERT_TRUE(request.url.is_valid()) << test_name;
          ASSERT_TRUE(ipfs::IsDefaultGatewayURL(request.url, GetPrefs()))
              << test_name;

          auto current_cid = GetCidFromUrl(request.url.spec());
          ASSERT_TRUE(current_cid) << test_name;

          ASSERT_TRUE(base::Contains(request.url.query(), "format=car"))
              << test_name;
          ASSERT_TRUE(base::Contains(request.url.query(), "dag-scope=entity"))
              << test_name;
          // ASSERT_TRUE(base::Contains(
          //     net::UnescapePercentEncodedUrl(request.url.query()),
          //     "entity-bytes=0:0")) << test_name;

          auto response_head = network::mojom::URLResponseHead::New();
          response_head->headers =
              base::MakeRefCounted<net::HttpResponseHeaders>("");

          response_head->headers->ReplaceStatusLine("HTTP/1.1 200 OK");
          const auto cids_to_car_map_iter = cids_to_car_map.find(*current_cid);
          LOG(INFO) << "[IPFS] current_cid:" << *current_cid;
          EXPECT_NE(cids_to_car_map_iter, cids_to_car_map.end()) << test_name;
          current_cid_file_test_data_ptr = &cids_to_car_map_iter->second;

          LOG(INFO) << "[IPFS] Interceptor url:" << request.url << " car_file_name:" << current_cid_file_test_data_ptr->car_file_name;
          auto content =
              GetFileContent(current_cid_file_test_data_ptr->car_file_name);
          url_loader_factory()->AddResponse(
              request.url, std::move(response_head), content,
              network::URLLoaderCompletionStatus(net::OK));
          LOG(INFO) << "[IPFS] Interceptor Finish url:" << request.url << " content.length:" << content.length();
        }));

    base::RunLoop run_loop;
    std::vector<uint8_t> received_data;
    int last_chunk_received_counter{0};
    auto orchestrator_request_callback = base::BindLambdaForTesting(
        [&](std::unique_ptr<IpfsTrustlessRequest> request,
            std::unique_ptr<IpfsTrustlessResponse> response) {
          ASSERT_TRUE(current_cid_file_test_data_ptr);
          EXPECT_TRUE(response) << test_name;
          EXPECT_TRUE(response->body) << test_name;
          received_data.insert(received_data.end(), response->body->begin(),
                               response->body->end());
          if (response->is_last_chunk) {
            last_chunk_received_counter++;
            run_loop.Quit();
          }
          LOG(INFO) << "[IPFS] total_size: " << response->total_size << " last_chunk_received_counter:" << last_chunk_received_counter;
          EXPECT_EQ(response->total_size, current_cid_file_test_data_ptr->size)
              << test_name;
        });
    orchestrator->BuildResponse(std::move(req),
                                std::move(orchestrator_request_callback));
    ASSERT_TRUE(orchestrator->IsActive()) << test_name;
    LOG(INFO) << "[IPFS] Wait for operation finish";
    //task_environment()->RunUntilIdle();
    run_loop.Run();
    ASSERT_TRUE(current_cid_file_test_data_ptr);
    LOG(INFO) << "[IPFS] current_cid_file_test_data_ptr->file_content:"
              << current_cid_file_test_data_ptr->file_content;
    EXPECT_TRUE(CompareVeAndStr(
        received_data,
        GetFileContent(current_cid_file_test_data_ptr->file_content)))
        << test_name;
    EXPECT_EQ(last_chunk_received_counter, 1) << test_name;

    orchestrator->Reset();
    ASSERT_FALSE(orchestrator->IsActive()) << test_name;
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::FilePath test_data_dir_;
  std::unique_ptr<TestingProfile> profile_;
  network::TestURLLoaderFactory url_loader_factory_;
};

TEST_F(BlockOrchestratorUnitTest, RequestCarContent) {
  base::ranges::for_each(
      kOneFileExtractInputData, [this](const auto& item_case) {
        TestGetCarFileByIpfsCid(item_case.test_name, item_case.ipfs_url,
                                item_case.cids_to_car_map);
      });
}

TEST_F(BlockOrchestratorUnitTest, ShardingRequestFile) {
  std::map<std::string, CarFileTestData> item_cases = {
      {"bafybeihn2f7lhumh4grizksi2fl233cyszqadkn424ptjajfenykpsaiw4",
       {"wiki_sharding_root.car", "", 0}
       },
       {"bafybeiff3a2xsr3sijmrauisyhhqztld5njl4z52k62zsjlqewiqwxyaie", {"wiki_sharding_A0_0.car", "", 0}},
       {"bafybeibrz7jl56wvr6hsrvygysnhuj2hzoscudleo24xhd37uc2b3qcwim", {"wiki_sharding_A0_1.car", "", 0}},
       {"bafybeihau4ajtl6l2v6teqxneyc7j3xefubh5rzdye6e7xkhitj5j5tbaq", {"wiki_sharding_A0_2.car", "", 0}},
       {"bafkreibpn742ynyqzjdabdsydshwb6jcv32nzbiggxyuftxdtvg5pxa2qm", {"wiki_sharding_A0_3.car", "wiki_sharding_A0_3.txt", 30593}}};
  TestGetCarFileByIpfsCid(
      "DirectoryHamSharding",
      "ipfs://bafybeihn2f7lhumh4grizksi2fl233cyszqadkn424ptjajfenykpsaiw4",
      item_cases);
}

// TEST_F(BlockOrchestratorUnitTest, RequestFolderGetIndexFile) {
//   const auto* test = &kOneFileExtractInputData[0];
//    TestGetCarFileByIpfsCid(
//        test->test_name,
//        test->ipfs_url,
//        test->cids_to_car_map);
// }
}  // namespace ipfs::ipld

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_reader.h"

#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include "base/containers/contains.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/test/bind.h"
#include "base/test/test_timeouts.h"
#include "base/values.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/ipld/block.h"
#include "brave/components/ipfs/ipld/car_block_reader.h"
#include "brave/components/ipfs/ipld/car_content_requester.h"
#include "brave/components/ipfs/ipld/content_requester.h"
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
  std::string cid;
  bool is_root;
  bool is_content;
  bool is_meta;
} kBlockCases[] = {
    {"bafkreifst3pqztuvj57lycamoi7z34b4emf7gawxs74nwrc2c7jncmpaqm", false, true,
     false},
    {"bafkreicll3huefkc3qnrzeony7zcfo7cr3nbx64hnxrqzsixpceg332fhe", false, true,
     false},
    {"bafkreigu7buvm3cfunb35766dn7tmqyh2um62zcio63en2btvxuybgcpue", false, true,
     false},
    {"bafkreih4ephajybraj6wnxsbwjwa77fukurtpl7oj7t7pfq545duhot7cq", false, true,
     false},
    {"bafkreie5noke3mb7hqxukzcy73nl23k6lxszxi5w3dtmuwz62wnvkpsscm", false, true,
     false},
    {"bafybeigcisqd7m5nf3qmuvjdbakl5bdnh4ocrmacaqkpuh77qjvggmt2sa", false,
     false, true},
    {"bafkreifjjcie6lypi6ny7amxnfftagclbuxndqonfipmb64f2km2devei4", false, true,
     false},
    {"bafkreifkam6ns4aoolg3wedr4uzrs3kvq66p4pecirz6y2vlrngla62mxm", false, true,
     false},
    {"bafybeicnmple4ehlz3ostv2sbojz3zhh5q7tz5r2qkfdpqfilgggeen7xm", false,
     false, true},
    {"bafybeidh6k2vzukelqtrjsmd4p52cpmltd2ufqrdtdg6yigi73in672fwu", false,
     false, true}};

constexpr char kDefaultIpfsUrl[] =
    "ipfs://bafybeigcisqd7m5nf3qmuvjdbakl5bdnh4ocrmacaqkpuh77qjvggmt2sa";

void EnumerateCarBlocks(
    std::unordered_map<std::string, std::unique_ptr<ipfs::ipld::Block>>&
        all_blocks,
    const std::string& cid_to_start) {
  std::stack<ipfs::ipld::Block*> blocks_stack;
  ipfs::ipld::Block* current = all_blocks[cid_to_start].get();

  while (current != nullptr || !blocks_stack.empty()) {
    if (current) {
      DCHECK(current != nullptr);
      blocks_stack.push(current);
    }

    while (current != nullptr) {
      if (!current->GetLinks()) {
        current = nullptr;
        continue;
      }

      for (const auto& item : *current->GetLinks()) {
        current = all_blocks[item.hash].get();
        DCHECK(current != nullptr);
        blocks_stack.push(current);
      }
    }

    current = blocks_stack.top();
    blocks_stack.pop();

    const auto* block_case =
        base::ranges::find_if(kBlockCases, [current](const auto& item_case) {
          return item_case.cid == current->Cid();
        });
    EXPECT_NE(block_case, base::ranges::end(kBlockCases));
    EXPECT_EQ(current->IsRoot(), block_case->is_root);
    EXPECT_EQ(current->IsContent(), block_case->is_content);
    EXPECT_EQ(current->IsMetadata(), block_case->is_meta);
    if (current->IsContent()) {
      EXPECT_GT(current->GetContentData()->size(), 0UL);
      EXPECT_TRUE(current->IsVerified().has_value());
      EXPECT_TRUE(current->IsVerified().value());
    } else {
      EXPECT_FALSE(current->IsVerified().has_value());
    }

    current = nullptr;
  }
}

void TestBlockExisting(
    std::unordered_map<std::string, std::unique_ptr<ipfs::ipld::Block>>&
        all_blocks) {
  ipfs::ipld::Block* header_block = all_blocks[""].get();
  auto* root_cids = header_block->Meta().Find("roots");
  EXPECT_TRUE(root_cids);

  EXPECT_TRUE(header_block->Cid().empty());
  EXPECT_TRUE(header_block->IsRoot() && !header_block->IsContent() &&
              !header_block->IsMetadata());
  EXPECT_EQ(root_cids->GetList().size(), 1UL);
  EXPECT_EQ(root_cids->GetList()[0].GetString(),
            "bafybeidh6k2vzukelqtrjsmd4p52cpmltd2ufqrdtdg6yigi73in672fwu");

  base::ranges::for_each(root_cids->GetList(),
                         [&all_blocks](const base::Value& item) {
                           EnumerateCarBlocks(all_blocks, item.GetString());
                         });
}

}  // namespace

namespace ipfs::ipld {

class BlockReaderUnitTest : public testing::Test {
 public:
  BlockReaderUnitTest() {
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    test_data_dir_ = test_data_dir_.AppendASCII(kTestDataSubDir);
  }
  ~BlockReaderUnitTest() override = default;

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

  using ChunkCarFetcherCallback =
      base::RepeatingCallback<void(std::unique_ptr<std::string>)>;
  void GetFileContentByChunks(const std::string& filename,
                              const unsigned int chunk_size,
                              ChunkCarFetcherCallback callback,
                              uint64_t& file_size) const {
    const auto full_path = test_data_dir_.AppendASCII(filename);
    uint64_t curr_pos = 0;
    std::string result;
    [&]() { ASSERT_TRUE(base::ReadFileToString(full_path, &result)); }();
    while (curr_pos < result.size()) {
      auto start_pos = result.begin() + curr_pos;
      auto end_pos = start_pos + ((curr_pos + chunk_size < result.size())
                                      ? chunk_size
                                      : (result.size() - curr_pos));
      callback.Run(std::make_unique<std::string>(start_pos, end_pos));
      curr_pos += end_pos - start_pos;
    }
    file_size = curr_pos;
  }

  std::string GetFileContent(const std::string& filename) const {
    std::string result;

    const auto full_path = test_data_dir_.AppendASCII(filename);
    /* void() for assert */
    [&]() { ASSERT_TRUE(base::ReadFileToString(full_path, &result)); }();
    return result;
  }

 private:
  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::FilePath test_data_dir_;
  std::unique_ptr<TestingProfile> profile_;
  network::TestURLLoaderFactory url_loader_factory_;
};

TEST_F(BlockReaderUnitTest, BasicTestSteps) {
  std::unordered_map<std::string, std::unique_ptr<ipfs::ipld::Block>>
      all_blocks;
  int32_t complete_counter{0};
  auto request_callback = base::BindLambdaForTesting(
      [&all_blocks, &complete_counter](std::unique_ptr<ipfs::ipld::Block> block,
                                       bool is_completed) {
        if (is_completed && !block) {
          complete_counter++;
          return;
        }
        all_blocks.try_emplace(block->Cid(), std::move(block));
      });
  url_loader_factory()->SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        ASSERT_TRUE(request.url.is_valid());
        ASSERT_TRUE(ipfs::IsDefaultGatewayURL(request.url, GetPrefs()));

        ASSERT_TRUE(base::Contains(request.url.query(), "format=car"));
        ASSERT_TRUE(base::Contains(request.url.query(), "dag-scope=entity"));
        ASSERT_TRUE(
            base::Contains(net::UnescapePercentEncodedUrl(request.url.query()),
                           "entity-bytes=0:0"));

        auto response_head = network::mojom::URLResponseHead::New();
        response_head->headers =
            base::MakeRefCounted<net::HttpResponseHeaders>("");
        response_head->headers->SetHeader(
            "Content-Type",
            "application/vnd.ipld.car; version=1; order=dfs; dups=n");
        response_head->headers->ReplaceStatusLine("HTTP/1.1 200 OK");
        url_loader_factory()->AddResponse(
            request.url, std::move(response_head),
            GetFileContent(kSubDirWithMixedBlockFiles),
            network::URLLoaderCompletionStatus(net::OK));
      }));
  GURL url(kDefaultIpfsUrl);
  auto br = std::make_unique<ipfs::ipld::CarBlockReader>(
      std::make_unique<ipfs::ipld::CarContentRequester>(
          url,
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              url_loader_factory()),
          GetPrefs()));
  br->Read(request_callback);
  task_environment()->RunUntilIdle();
  EXPECT_GT(all_blocks.size(), 0UL);
  EXPECT_FALSE(br->content_requester_->IsStarted());
  TestBlockExisting(all_blocks);
  EXPECT_EQ(complete_counter, 1);
}

TEST_F(BlockReaderUnitTest, ReceiveBlocksByChunks) {
  std::unordered_map<std::string, std::unique_ptr<ipfs::ipld::Block>>
      all_blocks;
  int32_t complete_counter{0};
  auto request_callback = base::BindLambdaForTesting(
      [&all_blocks, &complete_counter](std::unique_ptr<ipfs::ipld::Block> block,
                                       bool is_completed) {
        if (is_completed && !block) {
          complete_counter++;
          return;
        }

        all_blocks.try_emplace(block->Cid(), std::move(block));
      });
  GURL url(kDefaultIpfsUrl);
  auto br = std::make_unique<ipfs::ipld::CarBlockReader>(
      std::make_unique<ipfs::ipld::CarContentRequester>(
          url,
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              url_loader_factory()),
          GetPrefs()));
  br->content_requester_->buffer_ready_callback_ =
      br->GetReadCallbackForTests(std::move(request_callback));
  br->content_requester_->is_started_ = true;

  const uint32_t chunk_size = 100;
  uint64_t file_size = 0;
  uint64_t read_bytes = 0;
  GetFileContentByChunks(
      kSubDirWithMixedBlockFiles, chunk_size,
      base::BindLambdaForTesting([&](std::unique_ptr<std::string> data) {
        EXPECT_LE(data->size(), chunk_size);
        br->content_requester_->OnDataReceived(*data, base::NullCallback());
        read_bytes += data->size();
      }),
      file_size);
  EXPECT_TRUE(br->is_header_retrieved_);
  EXPECT_TRUE(br->buffer_.empty());

  br->content_requester_->OnComplete(true);
  EXPECT_FALSE(br->is_header_retrieved_);
  EXPECT_FALSE(br->content_requester_->IsStarted());
  EXPECT_EQ(read_bytes, file_size);
  TestBlockExisting(all_blocks);
  EXPECT_EQ(complete_counter, 1);
}

}  // namespace ipfs::ipld

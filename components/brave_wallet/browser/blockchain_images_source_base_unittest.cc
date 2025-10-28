/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_images_source_base.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BlockchainImagesSourceBaseTest : public testing::Test {
 public:
  BlockchainImagesSourceBaseTest(const BlockchainImagesSourceBaseTest&) =
      delete;
  BlockchainImagesSourceBaseTest& operator=(
      const BlockchainImagesSourceBaseTest&) = delete;

 protected:
  BlockchainImagesSourceBaseTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~BlockchainImagesSourceBaseTest() override = default;

  void SetUp() override {
    base::FilePath test_dir;
    // brave/test/data/brave_wallet/1.0.1
    brave_wallet::SetLastInstalledWalletVersionForTest(base::Version("1.0.1"));
    source_ = std::make_unique<BlockchainImagesSourceBase>(
        BraveWalletTestDataFolder());
  }
  void TearDown() override { source_.reset(); }

  BlockchainImagesSourceBase* source() const { return source_.get(); }

  bool data_received() const { return data_received_; }
  std::string data() const { return data_; }

  void StartRequest(GURL url) {
    data_received_ = false;
    data_.clear();
    const std::string path = content::URLDataSource::URLToRequestPath(url);
    source()->HandleImageRequest(
        path, base::BindOnce(&BlockchainImagesSourceBaseTest::OnDataReceived,
                             base::Unretained(this)));
  }

  content::BrowserTaskEnvironment task_environment_;

 private:
  void OnDataReceived(scoped_refptr<base::RefCountedMemory> bytes) {
    data_received_ = true;
    if (bytes) {
      data_ = std::string(std::string_view(
          reinterpret_cast<const char*>(bytes->front()), bytes->size()));
    }
  }

  std::unique_ptr<BlockchainImagesSourceBase> source_;
  bool data_received_ = false;
  std::string data_;
};

TEST_F(BlockchainImagesSourceBaseTest, GetMimeTypeForPath) {
  EXPECT_EQ(source()->GetMimeTypeForPath("brave://test/img1.png"), "image/png");
  EXPECT_EQ(source()->GetMimeTypeForPath("brave://test/img1.gif"), "image/gif");
  EXPECT_EQ(source()->GetMimeTypeForPath("brave://test/img1.jpg"), "image/jpg");
  EXPECT_EQ(source()->GetMimeTypeForPath("brave://test/img1.svg"),
            "image/svg+xml");
}

TEST_F(BlockchainImagesSourceBaseTest, HandleImageRequest) {
  StartRequest(GURL("chrome://erc-token-images/logo.png"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(data_received());
  EXPECT_FALSE(data().empty());
}

TEST_F(BlockchainImagesSourceBaseTest, HandleImageRequestImageNotExist) {
  StartRequest(GURL("chrome://erc-token-images/ent.svg"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(data_received());
  EXPECT_TRUE(data().empty());
}

}  // namespace brave_wallet

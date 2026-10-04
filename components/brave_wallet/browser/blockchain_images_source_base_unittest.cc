/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_images_source_base.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/version.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class TestBlockchainImagesSource : public BlockchainImagesSourceBase {
 public:
  explicit TestBlockchainImagesSource(const base::FilePath& base_path)
      : BlockchainImagesSourceBase(base_path) {}
  using BlockchainImagesSourceBase::GetMimeTypeForPath;
  using BlockchainImagesSourceBase::StartDataRequestForPath;
};

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
    // brave/components/test/data/brave_wallet/1.0.1
    brave_wallet::SetLastInstalledWalletVersionForTest(base::Version("1.0.1"));
    source_ = std::make_unique<TestBlockchainImagesSource>(
        BraveWalletComponentsTestDataFolder());
  }
  void TearDown() override {
    data_received_ = false;
    source_.reset();
    data_.reset();
  }

  TestBlockchainImagesSource* source() const { return source_.get(); }

  bool data_received() const { return data_received_; }
  scoped_refptr<base::RefCountedMemory> data() const { return data_; }

  base::test::TaskEnvironment task_environment_;

  void OnDataReceived(scoped_refptr<base::RefCountedMemory> bytes) {
    data_received_ = true;
    data_ = std::move(bytes);
  }

  std::unique_ptr<TestBlockchainImagesSource> source_;
  bool data_received_ = false;
  scoped_refptr<base::RefCountedMemory> data_;
  size_t data_size() const { return data_ ? data_->size() : 0; }
};

TEST_F(BlockchainImagesSourceBaseTest, GetMimeTypeForPath) {
  EXPECT_EQ(source()->GetMimeTypeForPath("img1.png"), "image/png");
  EXPECT_EQ(source()->GetMimeTypeForPath("img1.gif"), "image/gif");
  EXPECT_EQ(source()->GetMimeTypeForPath("img1.jpg"), "image/jpg");
  EXPECT_EQ(source()->GetMimeTypeForPath("img1.svg"), "image/svg+xml");
}

TEST_F(BlockchainImagesSourceBaseTest, StartDataRequestForPath) {
  base::RunLoop run_loop;

  source()->StartDataRequestForPath(
      "logo.png", base::BindLambdaForTesting(
                      [&](scoped_refptr<base::RefCountedMemory> bytes) {
                        OnDataReceived(std::move(bytes));
                        run_loop.Quit();
                      }));

  run_loop.Run();
  EXPECT_TRUE(data_received());
  EXPECT_TRUE(data());
  EXPECT_GT(data_size(), 0u);
}

TEST_F(BlockchainImagesSourceBaseTest, StartDataRequestForPathImageNotExist) {
  base::RunLoop run_loop;

  source()->StartDataRequestForPath(
      "ent.svg", base::BindLambdaForTesting(
                     [&](scoped_refptr<base::RefCountedMemory> bytes) {
                       OnDataReceived(std::move(bytes));
                       run_loop.Quit();
                     }));

  run_loop.Run();
  EXPECT_TRUE(data_received());
  EXPECT_FALSE(data());
}

}  // namespace brave_wallet

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/erc_token_images_source.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class ERCTokenImagesSourceTest : public testing::Test {
 protected:
  ERCTokenImagesSourceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~ERCTokenImagesSourceTest() override = default;

  void SetUp() override {
    base::PathService::Get(brave::DIR_TEST_DATA, &test_dir_);
    source_ = std::make_unique<ERCTokenImagesSource>(test_dir_);
  }
  void TearDown() override { source_.reset(); }

  ERCTokenImagesSource* source() const { return source_.get(); }

  bool data_received() const { return data_received_; }
  std::string data() const { return data_; }

  void StartRequest(GURL url) {
    data_received_ = false;
    data_.clear();
    content::WebContents::Getter wc_getter;
    source()->StartDataRequest(
        url, std::move(wc_getter),
        base::BindOnce(&ERCTokenImagesSourceTest::OnDataReceived,
                       base::Unretained(this)));
  }

  content::BrowserTaskEnvironment task_environment_;

 private:
  void OnDataReceived(scoped_refptr<base::RefCountedMemory> bytes) {
    data_received_ = true;
    if (bytes) {
      data_ = std::string(base::StringPiece(
          reinterpret_cast<const char*>(bytes->front()), bytes->size()));
    }
  }

  std::unique_ptr<ERCTokenImagesSource> source_;
  bool data_received_ = false;
  std::string data_;
  base::FilePath test_dir_;

  DISALLOW_COPY_AND_ASSIGN(ERCTokenImagesSourceTest);
};

TEST_F(ERCTokenImagesSourceTest, GetMimeType) {
  EXPECT_EQ(source()->GetMimeType("test/img1.png"), "image/png");
  EXPECT_EQ(source()->GetMimeType("test/img1.gif"), "image/gif");
  EXPECT_EQ(source()->GetMimeType("test/img1.jpg"), "image/jpg");
  EXPECT_EQ(source()->GetMimeType("test/img1.svg"), "image/svg+xml");
}

TEST_F(ERCTokenImagesSourceTest, StartDataRequest) {
  StartRequest(GURL("chrome://erc-token-images/logo.png"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(data_received());
  EXPECT_FALSE(data().empty());
}

TEST_F(ERCTokenImagesSourceTest, StartDataRequestImageNotExist) {
  StartRequest(GURL("chrome://erc-token-images/ent.svg"));
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(data_received());
  EXPECT_TRUE(data().empty());
}

}  // namespace brave_wallet

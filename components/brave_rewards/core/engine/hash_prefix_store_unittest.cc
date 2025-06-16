/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/hash_prefix_store.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span_writer.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_view_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {

class RewardsHashPrefixStoreTest : public testing::Test {
 public:
  void SetUp() override {
    Test::SetUp();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    store_path_ = temp_dir_.GetPath().AppendASCII("prefixes.store");
  }

  std::array<uint8_t, 16u> MakeFileHeader(uint32_t version,
                                          uint32_t prefix_size,
                                          uint64_t prefix_count) {
    std::array<uint8_t, 16u> header;
    base::SpanWriter writer(base::as_writable_byte_span(header));
    auto encoded_version = base::U32ToLittleEndian(version);
    writer.Write(base::as_byte_span(encoded_version));
    auto encoded_size = base::U32ToLittleEndian(prefix_size);
    writer.Write(base::as_byte_span(encoded_size));
    auto encoded_count = base::U64ToLittleEndian(prefix_count);
    writer.Write(base::as_byte_span(encoded_count));
    return header;
  }

  std::string MakePrefixString(size_t prefix_size,
                               std::vector<std::string_view> values) {
    std::vector<std::string> hash_values;
    for (auto& value : values) {
      std::string hash = crypto::SHA256HashString(value);
      hash.resize(prefix_size);
      hash_values.push_back(std::move(hash));
    }

    std::sort(hash_values.begin(), hash_values.end());

    std::string data;
    data.reserve(hash_values.size() * prefix_size);
    for (auto& value : hash_values) {
      data += value;
    }
    return data;
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath store_path_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(RewardsHashPrefixStoreTest, FileDoesNotExist) {
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
  EXPECT_FALSE(store.ContainsPrefix("test-value"));
}

TEST_F(RewardsHashPrefixStoreTest, EmptyFile) {
  base::WriteFile(store_path_, "");
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
  EXPECT_FALSE(store.ContainsPrefix("test-value"));
}

TEST_F(RewardsHashPrefixStoreTest, InvalidFileHeader) {
  base::WriteFile(store_path_, "--------");
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
  EXPECT_FALSE(store.ContainsPrefix("test-value"));
}

TEST_F(RewardsHashPrefixStoreTest, InvalidFileVersion) {
  auto header = MakeFileHeader(2, 4, 0);
  base::WriteFile(store_path_, base::as_byte_span(header));
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, InvalidPrefixSize) {
  auto header = MakeFileHeader(1, 1024, 0);
  base::WriteFile(store_path_, base::as_byte_span(header));
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, InvalidPrefixCount) {
  auto header = MakeFileHeader(1, 4, 1);
  auto content = std::string(base::as_string_view(header)) + "?";
  base::WriteFile(store_path_, content);
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, WrongPrefixCount) {
  auto header = MakeFileHeader(1, 4, 0);
  auto content = std::string(base::as_string_view(header)) + "????";
  base::WriteFile(store_path_, content);
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, InvalidDataLength) {
  auto header = MakeFileHeader(1, 4, 0);
  auto content = std::string(base::as_string_view(header)) + "?";
  base::WriteFile(store_path_, content);
  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, ZeroPrefixes) {
  auto header = MakeFileHeader(1, 4, 0);
  base::WriteFile(store_path_, base::as_byte_span(header));
  HashPrefixStore store(store_path_);
  EXPECT_TRUE(store.Open());
  EXPECT_FALSE(store.ContainsPrefix("test-value"));
}

TEST_F(RewardsHashPrefixStoreTest, WithPrefixes) {
  HashPrefixStore store(store_path_);

  size_t prefix_size = 4;

  std::string data = MakePrefixString(
      prefix_size, {"test-value-1", "test-value-2", "test-value-3",
                    "test-value-4", "test-value-5", "test-value-6",
                    "test-value-7", "test-value-8", "test-value-9"});

  bool updated = store.UpdatePrefixes(data, prefix_size);

  EXPECT_TRUE(updated);
  EXPECT_TRUE(store.ContainsPrefix("test-value-4"));
  EXPECT_TRUE(store.ContainsPrefix("test-value-1"));
  EXPECT_TRUE(store.ContainsPrefix("test-value-9"));
  EXPECT_FALSE(store.ContainsPrefix("test-value-0"));

  data = MakePrefixString(prefix_size, {"test-value-10"});
  updated = store.UpdatePrefixes(data, prefix_size);

  EXPECT_TRUE(updated);
  EXPECT_TRUE(store.ContainsPrefix("test-value-10"));
  EXPECT_FALSE(store.ContainsPrefix("test-value-1"));
}

TEST_F(RewardsHashPrefixStoreTest, UpdateEmpty) {
  HashPrefixStore store(store_path_);

  size_t prefix_size = 8;
  bool updated = store.UpdatePrefixes("", prefix_size);
  EXPECT_TRUE(updated);
  EXPECT_FALSE(store.ContainsPrefix("test-value-1"));

  std::string data = MakePrefixString(prefix_size, {"test-value-1"});
  updated = store.UpdatePrefixes(data, prefix_size);
  EXPECT_TRUE(updated);
  EXPECT_TRUE(store.ContainsPrefix("test-value-1"));
}

}  // namespace brave_rewards::internal

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
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/engine/publisher/flat/prefix_storage_generated.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/flatbuffers/src/include/flatbuffers/flatbuffer_builder.h"

namespace brave_rewards::internal {

namespace {
struct TestBuilder {
  void WriteToFile(const base::FilePath& path) {
    flatbuffers::FlatBufferBuilder builder;
    auto flat_offsets = builder.CreateVector(offsets);
    auto flat_suffixes = builder.CreateVector(suffixes);
    auto prefix_storage = rewards::flat::CreatePrefixStorage(
        builder, magic, prefix_size, flat_offsets, flat_suffixes);
    builder.Finish(prefix_storage);

    base::WriteFile(base::FilePath(path), builder.GetBufferSpan());
  }

  std::vector<uint32_t> offsets = std::vector<uint32_t>(256u * 256u, 0);
  std::vector<uint8_t> suffixes = {1, 1, 1, 1};
  uint32_t magic = 0xBEEF0001;
  uint32_t prefix_size = 4;
};

}  // namespace
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

TEST_F(RewardsHashPrefixStoreTest, InvalidMagic) {
  TestBuilder builder;
  builder.magic = 0xBAD;
  builder.WriteToFile(store_path_);

  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, InvalidPrefixSize) {
  TestBuilder builder;
  builder.prefix_size = 1024;
  builder.WriteToFile(store_path_);

  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, InvalidOffsetsSize) {
  TestBuilder builder;
  builder.offsets.resize(256u * 256u + 1);
  builder.WriteToFile(store_path_);

  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, BadSuffixesSize) {
  TestBuilder builder;
  builder.suffixes.resize(5);
  builder.WriteToFile(store_path_);

  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, OverflowOffsets) {
  TestBuilder builder;
  std::fill(builder.offsets.begin(), builder.offsets.end(), 0xFFFF);

  builder.WriteToFile(store_path_);

  HashPrefixStore store(store_path_);
  EXPECT_TRUE(store.Open());
  EXPECT_FALSE(store.ContainsPrefix("test-value"));
}

TEST_F(RewardsHashPrefixStoreTest, ZeroPrefixes) {
  TestBuilder builder;
  builder.suffixes.resize(0);
  builder.WriteToFile(store_path_);

  HashPrefixStore store(store_path_);
  EXPECT_FALSE(store.Open());
}

TEST_F(RewardsHashPrefixStoreTest, WithPrefixes) {
  HashPrefixStore store(store_path_);

  size_t prefix_size = 4;

  // Strings with SHA256 hash starting with "\xFF\xFF" (the last index).
  const std::string special1 = "xX6iZh5g2Asrm6OB";
  const std::string special2 = "Rhk7INooywKNNttX";
  ASSERT_EQ(crypto::SHA256HashString(special1).substr(0, 2), "\xFF\xFF");
  ASSERT_EQ(crypto::SHA256HashString(special2).substr(0, 2), "\xFF\xFF");

  std::string data = MakePrefixString(
      prefix_size, {"test-value-1", "test-value-2", "test-value-3",
                    "test-value-4", "test-value-5", "test-value-6",
                    "test-value-7", "test-value-8", "test-value-9", special1});

  bool updated = store.UpdatePrefixes(data, prefix_size);

  EXPECT_TRUE(updated);
  EXPECT_TRUE(store.ContainsPrefix("test-value-4"));
  EXPECT_TRUE(store.ContainsPrefix("test-value-1"));
  EXPECT_TRUE(store.ContainsPrefix("test-value-9"));
  EXPECT_TRUE(store.ContainsPrefix(special1));
  EXPECT_FALSE(store.ContainsPrefix("test-value-0"));
  EXPECT_FALSE(store.ContainsPrefix(special2));

  data = MakePrefixString(prefix_size, {"test-value-10"});
  updated = store.UpdatePrefixes(data, prefix_size);

  EXPECT_TRUE(updated);
  EXPECT_TRUE(store.ContainsPrefix("test-value-10"));
  EXPECT_FALSE(store.ContainsPrefix("test-value-1"));
  EXPECT_FALSE(store.ContainsPrefix(special1));
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

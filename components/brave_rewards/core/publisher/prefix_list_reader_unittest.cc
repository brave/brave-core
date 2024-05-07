/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <array>
#include <utility>

#include "brave/components/brave_rewards/core/publisher/prefix_list_reader.h"
#include "brave/components/brave_rewards/core/publisher/protos/publisher_prefix_list.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {
namespace publisher {

class RewardsPrefixListReaderTest : public testing::Test {
 protected:
  template <typename F>
  PrefixListReader::ParseError TestParse(F init) {
    publishers_pb::PublisherPrefixList message;
    message.set_prefix_size(4);
    init(&message);

    std::string serialized;
    message.SerializeToString(&serialized);

    PrefixListReader reader;
    return reader.Parse(serialized);
  }
};

TEST_F(RewardsPrefixListReaderTest, ValidInput) {
  size_t prefix_size = 4;

  // A sorted list of prefixes. Note that actual prefixes
  // are raw bytes and not ascii chars.
  std::string prefix_data =
      "andy"
      "bear"
      "cake"
      "dear";

  publishers_pb::PublisherPrefixList list;
  list.set_prefix_size(prefix_size);
  list.set_compression_type(publishers_pb::PublisherPrefixList::NO_COMPRESSION);
  list.set_uncompressed_size(prefix_data.length());
  list.set_prefixes(prefix_data);

  std::string serialized;
  ASSERT_TRUE(list.SerializeToString(&serialized));

  PrefixListReader reader;

  // Basic successful parsing
  ASSERT_EQ(reader.Parse(serialized), PrefixListReader::ParseError::kNone);

  EXPECT_EQ(reader.size(), size_t(4));
  EXPECT_FALSE(reader.empty());

  // Iteration
  size_t offset = 0;
  for (auto prefix : reader) {
    EXPECT_EQ(prefix, prefix_data.substr(offset, prefix_size));
    offset += prefix_size;
  }

  // Binary searching
  EXPECT_TRUE(std::binary_search(reader.begin(), reader.end(), "cake"));
  EXPECT_FALSE(std::binary_search(reader.begin(), reader.end(), "pool"));

  // Move-construction
  PrefixListReader reader2(std::move(reader));
  EXPECT_TRUE(reader.empty());
  EXPECT_EQ(reader2.size(), size_t(4));

  // Move-assignment
  PrefixListReader reader3;
  reader3 = std::move(reader2);
  EXPECT_TRUE(reader2.empty());
  EXPECT_EQ(reader3.size(), size_t(4));
}

TEST_F(RewardsPrefixListReaderTest, InvalidInput) {
  PrefixListReader reader;
  ASSERT_EQ(reader.Parse("invalid input"),
            PrefixListReader::ParseError::kInvalidProtobufMessage);

  ASSERT_EQ(TestParse([](auto* list) { list->set_prefix_size(0); }),
            PrefixListReader::ParseError::kInvalidPrefixSize);

  ASSERT_EQ(TestParse([](auto* list) { list->set_prefix_size(3); }),
            PrefixListReader::ParseError::kInvalidPrefixSize);

  ASSERT_EQ(TestParse([](auto* list) { list->set_prefix_size(33); }),
            PrefixListReader::ParseError::kInvalidPrefixSize);

  ASSERT_EQ(TestParse([](auto* list) { list->set_uncompressed_size(0); }),
            PrefixListReader::ParseError::kInvalidUncompressedSize);

  ASSERT_EQ(TestParse([](auto* list) {
              list->set_prefixes("-----");
              list->set_uncompressed_size(5);
            }),
            PrefixListReader::ParseError::kInvalidUncompressedSize);

  ASSERT_EQ(
      TestParse([](auto* list) {
        list->set_prefixes("----");
        list->set_uncompressed_size(4);
        list->set_compression_type(
            static_cast<publishers_pb::PublisherPrefixList::CompressionType>(
                1000));
      }),
      PrefixListReader::ParseError::kUnknownCompressionType);

  ASSERT_EQ(TestParse([](auto* list) {
              list->set_prefixes("aaaabbbbzzzzcccc");
              list->set_uncompressed_size(16);
            }),
            PrefixListReader::ParseError::kPrefixesNotSorted);
}

TEST_F(RewardsPrefixListReaderTest, BrotliCompression) {
  ASSERT_EQ(TestParse([](auto* list) {
              list->set_uncompressed_size(16);
              list->set_compression_type(
                  publishers_pb::PublisherPrefixList::BROTLI_COMPRESSION);
            }),
            PrefixListReader::ParseError::kUnableToDecompress);

  constexpr std::array<uint8_t, 26> compressed = {
      0x1b, 0x1f, 0x00, 0xf8, 0xc5, 0x1,  0xc7, 0x80, 0xb8,
      0xbe, 0x44, 0x89, 0x28, 0x10, 0x78, 0x0,  0x20, 0x49,
      0x49, 0xb2, 0xed, 0x24, 0x69, 0xdb, 0xf9, 0x7f,
  };

  std::string prefixes(compressed.begin(), compressed.end());

  publishers_pb::PublisherPrefixList list;
  list.set_prefix_size(4);
  list.set_compression_type(
      publishers_pb::PublisherPrefixList::BROTLI_COMPRESSION);
  list.set_uncompressed_size(32);
  list.set_prefixes(prefixes);

  std::string serialized;
  ASSERT_TRUE(list.SerializeToString(&serialized));

  PrefixListReader reader;

  ASSERT_EQ(reader.Parse(serialized), PrefixListReader::ParseError::kNone);

  std::string uncompressed;
  for (auto prefix : reader) {
    uncompressed.append(prefix.data(), prefix.length());
  }

  ASSERT_EQ(uncompressed, "aaaabbbbccccddddeeeeffffgggghhhh");
}

}  // namespace publisher
}  // namespace brave_rewards::internal

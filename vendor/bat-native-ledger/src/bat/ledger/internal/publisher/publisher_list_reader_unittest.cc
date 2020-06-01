/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "bat/ledger/internal/publisher/publisher_list_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter='PublisherListReaderTest.*'

using publishers_pb::PublisherList;

namespace braveledger_publisher {

class PublisherListReaderTest : public testing::Test {
 protected:
  template<typename F>
  PublisherListReader::ParseError TestParse(F init) {
    PublisherList message;
    message.set_prefix_size(4);
    init(&message);

    std::string serialized;
    message.SerializeToString(&serialized);

    PublisherListReader reader;
    return reader.Parse(serialized);
  }
};

TEST_F(PublisherListReaderTest, ValidInput) {
  size_t prefix_size = 4;

  // A sorted list of prefixes. Note that actual prefixes
  // are raw bytes and not chars.
  std::string prefix_data =
    "andy"
    "bear"
    "cake"
    "dear";

  PublisherList list;
  list.set_prefix_size(prefix_size);
  list.set_compression_type(PublisherList::NO_COMPRESSION);
  list.set_uncompressed_size(prefix_data.length());
  list.set_prefixes(prefix_data);

  std::string serialized;
  ASSERT_TRUE(list.SerializeToString(&serialized));

  PublisherListReader reader;

  // Basic successful parsing
  ASSERT_EQ(
      reader.Parse(serialized),
      PublisherListReader::ParseError::None);

  // Iteration
  size_t offset = 0;
  for (auto prefix : reader) {
    EXPECT_EQ(prefix, prefix_data.substr(offset, prefix_size));
    offset += prefix_size;
  }

  // Binary searching
  EXPECT_TRUE(std::binary_search(reader.begin(), reader.end(), "cake"));
  EXPECT_FALSE(std::binary_search(reader.begin(), reader.end(), "pool"));
}

TEST_F(PublisherListReaderTest, InvalidInput) {
  PublisherListReader reader;
  ASSERT_EQ(
      reader.Parse("invalid input"),
      PublisherListReader::ParseError::InvalidProtobufMessage);

  ASSERT_EQ(
      TestParse([](auto* list) { list->set_prefix_size(0); }),
      PublisherListReader::ParseError::InvalidPrefixSize);

  ASSERT_EQ(
      TestParse([](auto* list) { list->set_prefix_size(3); }),
      PublisherListReader::ParseError::InvalidPrefixSize);

  ASSERT_EQ(
      TestParse([](auto* list) { list->set_prefix_size(33); }),
      PublisherListReader::ParseError::InvalidPrefixSize);

  ASSERT_EQ(
      TestParse([](auto* list) { list->set_uncompressed_size(0); }),
      PublisherListReader::ParseError::InvalidUncompressedSize);

  ASSERT_EQ(
      TestParse([](auto* list) {
        list->set_prefixes("-----");
        list->set_uncompressed_size(5);
      }),
      PublisherListReader::ParseError::InvalidUncompressedSize);

  ASSERT_EQ(
      TestParse([](auto* list) {
        list->set_prefixes("----");
        list->set_uncompressed_size(4);
        list->set_compression_type(
           static_cast<PublisherList::CompressionType>(1000));
      }),
      PublisherListReader::ParseError::UnknownCompressionType);

  ASSERT_EQ(
      TestParse([](auto* list) {
        list->set_prefixes("aaaabbbbzzzzcccc");
        list->set_uncompressed_size(16);
      }),
      PublisherListReader::ParseError::PrefixesNotSorted);
}

}  // namespace braveledger_publisher

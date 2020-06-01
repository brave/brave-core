/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/publisher/publisher_list_reader.h"

#include <utility>

#include "bat/ledger/internal/publisher/prefix_util.h"

namespace braveledger_publisher {

PublisherListReader::PublisherListReader()
    : prefix_size_(kMinPrefixSize) {}

PublisherListReader::~PublisherListReader() = default;

PublisherListReader::ParseError PublisherListReader::Parse(
    const std::string& contents) {
  using publishers_pb::PublisherList;

  PublisherList message;
  if (!message.ParseFromString(contents)) {
    return ParseError::InvalidProtobufMessage;
  }

  const size_t prefix_size = message.prefix_size();
  if (prefix_size < kMinPrefixSize || prefix_size > kMaxPrefixSize) {
    return ParseError::InvalidPrefixSize;
  }

  const size_t uncompressed_size = message.uncompressed_size();
  if (uncompressed_size == 0) {
    return ParseError::InvalidUncompressedSize;
  }

  std::string uncompressed;
  switch (message.compression_type()) {
    case PublisherList::NO_COMPRESSION: {
      uncompressed = std::move(*message.mutable_prefixes());
      break;
    }
    default: {
      return ParseError::UnknownCompressionType;
    }
  }

  if (uncompressed.size() % prefix_size != 0) {
    return ParseError::InvalidUncompressedSize;
  }

  prefixes_ = std::move(uncompressed);
  prefix_size_ = prefix_size;

  // Perform a quick sanity check that the first few prefixes are in order.
  PrefixIterator iter = this->begin();
  if (iter != this->end()) {
    for (size_t i = 0; i < 5; ++i) {
      auto next = iter + 1;
      if (next == this->end()) {
        break;
      }
      if (*iter > *next) {
        prefixes_ = "";
        return ParseError::PrefixesNotSorted;
      }
      iter = next;
    }
  }

  return ParseError::None;
}

}  // namespace braveledger_publisher

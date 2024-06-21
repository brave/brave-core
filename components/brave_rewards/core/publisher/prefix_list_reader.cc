/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/prefix_list_reader.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/brotli_util.h"
#include "brave/components/brave_rewards/core/publisher/prefix_util.h"
#include "brave/components/brave_rewards/core/publisher/protos/publisher_prefix_list.pb.h"

namespace brave_rewards::internal::publisher {

PrefixListReader::PrefixListReader() : prefix_size_(kMinPrefixSize) {}

PrefixListReader::PrefixListReader(PrefixListReader&& other)
    : prefix_size_(other.prefix_size_), prefixes_(std::move(other.prefixes_)) {}

PrefixListReader& PrefixListReader::operator=(PrefixListReader&& other) {
  if (&other != this) {
    this->prefix_size_ = other.prefix_size_;
    this->prefixes_ = std::move(other.prefixes_);
  }
  return *this;
}

PrefixListReader::~PrefixListReader() = default;

PrefixListReader::ParseError PrefixListReader::Parse(
    const std::string& contents) {
  publishers_pb::PublisherPrefixList message;
  if (!message.ParseFromString(contents)) {
    return ParseError::kInvalidProtobufMessage;
  }

  const size_t prefix_size = message.prefix_size();
  if (prefix_size < kMinPrefixSize || prefix_size > kMaxPrefixSize) {
    return ParseError::kInvalidPrefixSize;
  }

  const size_t uncompressed_size = message.uncompressed_size();
  if (uncompressed_size == 0) {
    return ParseError::kInvalidUncompressedSize;
  }

  std::string uncompressed;
  switch (message.compression_type()) {
    case publishers_pb::PublisherPrefixList::NO_COMPRESSION: {
      uncompressed = std::move(*message.mutable_prefixes());
      break;
    }
    case publishers_pb::PublisherPrefixList::BROTLI_COMPRESSION: {
      bool decoded = util::DecodeBrotliString(message.prefixes(),
                                              uncompressed_size, &uncompressed);

      if (!decoded) {
        return ParseError::kUnableToDecompress;
      }
      break;
    }
    default: {
      return ParseError::kUnknownCompressionType;
    }
  }

  if (uncompressed.size() % prefix_size != 0) {
    return ParseError::kInvalidUncompressedSize;
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
        return ParseError::kPrefixesNotSorted;
      }
      iter = next;
    }
  }

  return ParseError::kNone;
}

}  // namespace brave_rewards::internal::publisher

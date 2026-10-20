// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TEST_TEST_VECTORS_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TEST_TEST_VECTORS_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/values.h"
#include "brave/components/brave_transfer/core/brave_transfer.pb.h"
#include "brave/components/brave_transfer/core/frame_codec.h"
#include "brave/components/brave_transfer/core/transfer_offer.h"

// Helpers for the shared vectors in components/brave_transfer/core/test/data,
// which the Swift and Python implementations also check.
namespace brave_transfer::test {

// Loads test/data/`file_name`. Fails the test if it cannot be read.
base::DictValue LoadVectors(std::string_view file_name);

Bytes FromHex(std::string_view hex);
std::string HexString(base::span<const uint8_t> bytes);
std::string HexString(std::string_view bytes);

// Byte i is i % 251.
Bytes Pattern(size_t length);

// JSON numbers above 2^31 are parsed as doubles.
uint64_t ToU64(const base::Value* value);

MessageType MessageTypeFromName(std::string_view name);
std::unique_ptr<google::protobuf::MessageLite> NewMessage(MessageType type);
// Builds a message from its JSON form in the vectors, where bytes fields are
// hex strings and nested messages are objects.
std::unique_ptr<google::protobuf::MessageLite> MessageFromJson(
    MessageType type,
    const base::DictValue& value);

proto::PeerInfo PeerInfoFromJson(const base::DictValue& value);
proto::OfferItem OfferItemFromJson(const base::DictValue& value);
proto::Offer OfferFromJson(const base::DictValue& value);
proto::Frame FrameFromJson(const base::DictValue& value);

// The "items" of session_transcript.json.
std::vector<OutgoingItem> TranscriptItems(const base::DictValue& transcript);

}  // namespace brave_transfer::test

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TEST_TEST_VECTORS_H_

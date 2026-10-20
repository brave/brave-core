// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TRANSFER_OFFER_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TRANSFER_OFFER_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "brave/components/brave_transfer/core/brave_transfer.pb.h"
#include "brave/components/brave_transfer/core/noise_handshake.h"

namespace brave_transfer {

// Transfer limits and timers (PROTOCOL.md §8).
inline constexpr size_t kTransferIdSize = 16;
inline constexpr size_t kMaxItems = 256;
inline constexpr size_t kMaxNameBytes = 1024;
inline constexpr size_t kMaxMimeTypeBytes = 255;
inline constexpr size_t kMaxInlineBytes = 8192;
inline constexpr uint64_t kMaxItemSize = uint64_t{1} << 40;
inline constexpr size_t kMaxChunkData = 61440;
inline constexpr base::TimeDelta kOfferTimeout = base::Seconds(60);
inline constexpr base::TimeDelta kIdleTimeout = base::Seconds(30);
inline constexpr base::TimeDelta kPingInterval = base::Seconds(10);

// Limits a receiver applies on top of the protocol maximums.
struct ReceiveLimits {
  size_t max_items = kMaxItems;
  uint64_t max_total_size = kMaxItemSize;
};

// Something to send. Small TEXT and URL items travel inline; everything else
// is read in chunks through `read`, which returns exactly `length` bytes at
// `offset`, or nullopt on failure.
struct OutgoingItem {
  using ReadCallback =
      base::RepeatingCallback<std::optional<Bytes>(uint64_t offset,
                                                   size_t length)>;

  OutgoingItem();
  OutgoingItem(const OutgoingItem&);
  OutgoingItem& operator=(const OutgoingItem&);
  OutgoingItem(OutgoingItem&&);
  OutgoingItem& operator=(OutgoingItem&&);
  ~OutgoingItem();

  // An item whose content is in memory. TEXT and URL items up to
  // kMaxInlineBytes are sent inline.
  static OutgoingItem FromBytes(proto::ItemKind kind,
                                std::string name,
                                std::string mime_type,
                                Bytes data);

  proto::ItemKind kind = proto::ITEM_KIND_FILE;
  std::string name;
  std::string mime_type;
  uint64_t size = 0;
  Bytes sha256;
  Bytes inline_data;
  ReadCallback read;
};

// Checks an offer against PROTOCOL.md §8.2.
Result<void> ValidateOffer(const proto::Offer& offer,
                           const ReceiveLimits& limits = ReceiveLimits());

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_TRANSFER_OFFER_H_

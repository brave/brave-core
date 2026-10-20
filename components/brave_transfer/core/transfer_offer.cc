// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/transfer_offer.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "crypto/hash.h"

namespace brave_transfer {

namespace {

bool IsTextual(proto::ItemKind kind) {
  return kind == proto::ITEM_KIND_TEXT || kind == proto::ITEM_KIND_URL;
}

bool IsKnownKind(int kind) {
  return kind == proto::ITEM_KIND_FILE || kind == proto::ITEM_KIND_IMAGE ||
         kind == proto::ITEM_KIND_TEXT || kind == proto::ITEM_KIND_URL;
}

std::optional<Bytes> ReadFromMemory(const Bytes& data,
                                    uint64_t offset,
                                    size_t length) {
  if (offset > data.size() || length > data.size() - offset) {
    return std::nullopt;
  }
  return base::ToVector(
      base::span(data).subspan(static_cast<size_t>(offset), length));
}

}  // namespace

OutgoingItem::OutgoingItem() = default;
OutgoingItem::OutgoingItem(const OutgoingItem&) = default;
OutgoingItem& OutgoingItem::operator=(const OutgoingItem&) = default;
OutgoingItem::OutgoingItem(OutgoingItem&&) = default;
OutgoingItem& OutgoingItem::operator=(OutgoingItem&&) = default;
OutgoingItem::~OutgoingItem() = default;

// static
OutgoingItem OutgoingItem::FromBytes(proto::ItemKind kind,
                                     std::string name,
                                     std::string mime_type,
                                     Bytes data) {
  OutgoingItem item;
  item.kind = kind;
  item.name = std::move(name);
  item.mime_type = std::move(mime_type);
  item.size = data.size();
  item.sha256 = base::ToVector(crypto::hash::Sha256(data));
  if (IsTextual(kind) && data.size() <= kMaxInlineBytes) {
    item.inline_data = std::move(data);
  } else {
    item.read = base::BindRepeating(&ReadFromMemory, std::move(data));
  }
  return item;
}

Result<void> ValidateOffer(const proto::Offer& offer,
                           const ReceiveLimits& limits) {
  if (offer.transfer_id().size() != kTransferIdSize) {
    return base::unexpected("bad transfer id");
  }
  const size_t count = static_cast<size_t>(offer.items_size());
  if (count < 1 || count > limits.max_items) {
    return base::unexpected("bad item count");
  }
  base::flat_set<uint32_t> ids;
  uint64_t total = 0;
  for (const proto::OfferItem& item : offer.items()) {
    if (!ids.insert(item.item_id()).second) {
      return base::unexpected("duplicate item id");
    }
    if (!IsKnownKind(item.kind())) {
      return base::unexpected("bad item kind");
    }
    if (item.name().empty() || item.name().size() > kMaxNameBytes) {
      return base::unexpected("bad item name");
    }
    if (item.mime_type().size() > kMaxMimeTypeBytes) {
      return base::unexpected("bad mime type");
    }
    if (item.sha256().size() != 32) {
      return base::unexpected("bad sha256");
    }
    if (item.size() > kMaxItemSize) {
      return base::unexpected("item too large");
    }
    total += item.size();
    const bool textual = IsTextual(item.kind());
    if (textual && item.size() == 0) {
      return base::unexpected("empty text item");
    }
    const bool must_inline =
        item.kind() == proto::ITEM_KIND_URL ||
        (textual && item.size() <= kMaxInlineBytes);
    if (!item.inline_data().empty()) {
      if (!must_inline || item.inline_data().size() != item.size()) {
        return base::unexpected("bad inline data");
      }
      const auto digest =
          crypto::hash::Sha256(base::as_byte_span(item.inline_data()));
      if (base::span(digest) != base::as_byte_span(item.sha256())) {
        return base::unexpected("inline data hash mismatch");
      }
    } else if (must_inline) {
      return base::unexpected("missing inline data");
    }
    if (item.kind() == proto::ITEM_KIND_URL) {
      if (!base::IsStringUTF8AllowingNoncharacters(item.inline_data())) {
        return base::unexpected("url is not UTF-8");
      }
      if (!base::StartsWith(item.inline_data(), "http://",
                            base::CompareCase::INSENSITIVE_ASCII) &&
          !base::StartsWith(item.inline_data(), "https://",
                            base::CompareCase::INSENSITIVE_ASCII)) {
        return base::unexpected("url scheme not allowed");
      }
    } else if (item.kind() == proto::ITEM_KIND_TEXT &&
               !item.inline_data().empty() &&
               !base::IsStringUTF8AllowingNoncharacters(item.inline_data())) {
      return base::unexpected("text is not UTF-8");
    }
  }
  if (total > limits.max_total_size) {
    return base::unexpected("offer too large");
  }
  return base::ok();
}

}  // namespace brave_transfer

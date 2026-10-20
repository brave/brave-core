// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/transfer_offer.h"

#include <string>

#include "brave/components/brave_transfer/core/test/test_vectors.h"
#include "crypto/hash.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

proto::Offer OfferFromCase(const base::DictValue& c) {
  proto::Offer offer = test::OfferFromJson(*c.FindDict("offer"));
  if (const base::DictValue* item_template = c.FindDict("item_template")) {
    proto::OfferItem item = test::OfferItemFromJson(*item_template);
    const uint64_t count = test::ToU64(c.Find("item_count"));
    offer.clear_items();
    for (uint64_t id = 1; id <= count; ++id) {
      proto::OfferItem* copy = offer.add_items();
      *copy = item;
      copy->set_item_id(static_cast<uint32_t>(id));
    }
  }
  return offer;
}

proto::Offer OneFileOffer(uint64_t size) {
  proto::Offer offer;
  offer.set_transfer_id(std::string(kTransferIdSize, '\x01'));
  proto::OfferItem* item = offer.add_items();
  item->set_item_id(1);
  item->set_kind(proto::ITEM_KIND_FILE);
  item->set_name("file.bin");
  item->set_mime_type("application/octet-stream");
  item->set_size(size);
  item->set_sha256(std::string(32, '\x02'));
  return offer;
}

}  // namespace

TEST(BraveTransferOfferTest, ValidationVectors) {
  base::DictValue vectors = test::LoadVectors("offer_validation.json");
  const base::ListValue* cases = vectors.FindList("cases");
  ASSERT_TRUE(cases);
  ASSERT_FALSE(cases->empty());
  for (const base::Value& entry : *cases) {
    const base::DictValue& c = entry.GetDict();
    SCOPED_TRACE(*c.FindString("name"));
    auto result = ValidateOffer(OfferFromCase(c));
    if (c.FindBool("valid").value_or(false)) {
      EXPECT_TRUE(result.has_value()) << result.error();
    } else {
      EXPECT_FALSE(result.has_value());
    }
  }
}

TEST(BraveTransferOfferTest, ReceiveLimits) {
  ReceiveLimits limits;
  limits.max_total_size = 1000;
  EXPECT_TRUE(ValidateOffer(OneFileOffer(1000), limits).has_value());
  EXPECT_FALSE(ValidateOffer(OneFileOffer(1001), limits).has_value());

  proto::Offer two = OneFileOffer(10);
  *two.add_items() = two.items(0);
  two.mutable_items(1)->set_item_id(2);
  limits = ReceiveLimits();
  EXPECT_TRUE(ValidateOffer(two, limits).has_value());
  limits.max_items = 1;
  EXPECT_FALSE(ValidateOffer(two, limits).has_value());
}

TEST(BraveTransferOfferTest, OutgoingItemFromBytes) {
  const std::string text = "https://brave.com";
  OutgoingItem url =
      OutgoingItem::FromBytes(proto::ITEM_KIND_URL, "brave.com",
                              "text/uri-list", Bytes(text.begin(), text.end()));
  EXPECT_EQ(url.size, text.size());
  EXPECT_EQ(test::HexString(url.sha256),
            test::HexString(crypto::hash::Sha256(base::as_byte_span(text))));
  EXPECT_EQ(url.inline_data, Bytes(text.begin(), text.end()));

  OutgoingItem big_text =
      OutgoingItem::FromBytes(proto::ITEM_KIND_TEXT, "a.txt", "text/plain",
                              Bytes(kMaxInlineBytes + 1, 'a'));
  EXPECT_TRUE(big_text.inline_data.empty());
  ASSERT_TRUE(big_text.read);
  auto chunk = big_text.read.Run(kMaxInlineBytes - 1, 2);
  ASSERT_TRUE(chunk);
  EXPECT_EQ(*chunk, Bytes(2, 'a'));
  EXPECT_FALSE(big_text.read.Run(kMaxInlineBytes, 2));

  OutgoingItem file =
      OutgoingItem::FromBytes(proto::ITEM_KIND_FILE, "f.bin",
                              "application/octet-stream", test::Pattern(10));
  EXPECT_TRUE(file.inline_data.empty());
  ASSERT_TRUE(file.read);
  EXPECT_EQ(*file.read.Run(0, 10), test::Pattern(10));
}

}  // namespace brave_transfer

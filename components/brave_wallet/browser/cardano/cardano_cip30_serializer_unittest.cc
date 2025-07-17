/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
CardanoAddress GetMockCardanoAddress() {
  return *CardanoAddress::FromString(
      "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8"
      "cc3sq835lu7drv2xwl2wywfgse35a3x");
}
}  // namespace

TEST(CardanoCip30SerializerTest, SerializedSignPayload) {
  EXPECT_EQ(
      "846A5369676E6174757265315882A30127235839019493315CD92EB5D8C4304E67B7E16A"
      "E36D61D34502694657811A2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628C"
      "EFA9C4725167616464726573735839019493315CD92EB5D8C4304E67B7E16AE36D61D345"
      "02694657811A2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251"
      "40456272617665",
      base::HexEncode(CardanoCip30Serializer::SerializedSignPayload(
          GetMockCardanoAddress(), base::byte_span_from_cstring("brave"))));
}

TEST(CardanoCip30SerializerTest, SerializeSignedDataKey) {
  EXPECT_EQ(
      "A50101025839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E33"
      "7B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251032720062146707562"
      "6B6579",
      base::HexEncode(CardanoCip30Serializer::SerializeSignedDataKey(
          GetMockCardanoAddress(), base::byte_span_from_cstring("pubkey"))));
}

TEST(CardanoCip30SerializerTest, SerializeSignedDataSignature) {
  EXPECT_EQ(
      "845882A30127235839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A"
      "2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251676164647265"
      "73735839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E337B62"
      "CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251A166686173686564F44562"
      "72617665497369676E6174757265",
      base::HexEncode(CardanoCip30Serializer::SerializeSignedDataSignature(
          GetMockCardanoAddress(), base::byte_span_from_cstring("brave"),
          base::byte_span_from_cstring("signature"))));
}

}  // namespace brave_wallet

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"

#include "base/containers/span.h"
#include "base/containers/span_writer.h"
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
      "846A5369676E6174757265315882A30127045839019493315CD92EB5D8C4304E67B7E16A"
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
      "845882A30127045839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A"
      "2C8E337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251676164647265"
      "73735839019493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E337B62"
      "CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251A166686173686564F44562"
      "72617665497369676E6174757265",
      base::HexEncode(CardanoCip30Serializer::SerializeSignedDataSignature(
          GetMockCardanoAddress(), base::byte_span_from_cstring("brave"),
          base::byte_span_from_cstring("signature"))));
}

TEST(CardanoCip30SerializerTest, SerializeAmount) {
  auto serialized = *CardanoCip30Serializer::SerializeAmount(2000000u);
  EXPECT_EQ(serialized, "1a001e8480");
  EXPECT_EQ(2000000u,
            CardanoCip30Serializer::DeserializeAmount(serialized).value());
  EXPECT_FALSE(CardanoCip30Serializer::DeserializeAmount(""));

  auto max_serialized = CardanoCip30Serializer::SerializeAmount(UINT64_MAX);
  EXPECT_FALSE(max_serialized);

  auto signed_max_serialized =
      *CardanoCip30Serializer::SerializeAmount(UINT64_MAX / 2);
  EXPECT_EQ(signed_max_serialized, "1b7fffffffffffffff");
  EXPECT_EQ(
      UINT64_MAX / 2,
      CardanoCip30Serializer::DeserializeAmount(signed_max_serialized).value());
}

TEST(CardanoCip30SerializerTest, SerializeUtxos) {
  cardano_rpc::UnspentOutputs utxos;
  // Utxo1
  {
    cardano_rpc::UnspentOutput output;
    output.output_index = 0;
    output.lovelace_amount = 1000000u;
    std::vector<uint8_t> tx_bytes;
    base::HexStringToBytes(
        "d9ef8dcd983c6fe996d5029e010e224bec191d0f63ff695cdab046abfd79dfbd",
        &tx_bytes);
    base::SpanWriter(base::span(output.tx_hash)).Write(tx_bytes);
    output.address_to = *CardanoAddress::FromString(
        "addr1qyx2zscdearcexdktcgq6g27jkyff65dw82w6catczfwxz2qjy"
        "nwf42y3c7ejrrekj5r2fh6kx5m9gcrmywpqxw3np5qjeh38p");
    utxos.push_back(std::move(output));
  }

  // Utxo2
  {
    cardano_rpc::UnspentOutput output;
    output.output_index = 0;
    output.lovelace_amount = 2000000u;
    std::vector<uint8_t> tx_bytes;
    base::HexStringToBytes(
        "42c7b97f09cf640dcb76c7426c1181594dfc2da3aa000476aa9639bc0a131f4d",
        &tx_bytes);
    base::SpanWriter(base::span(output.tx_hash)).Write(tx_bytes);
    output.address_to = *CardanoAddress::FromString(
        "addr1q95842gcg7yr4uxqrr0l389msd68rgvv7cd9q9qc9f36mddy"
        "q3v4daq49vspumzngv66wfydv2l3qsqtlwa2pvpd6vmstarkzs");
    utxos.push_back(std::move(output));
  }

  EXPECT_EQ(
      std::vector<std::string>(
          {"82825820d9ef8dcd983c6fe996d5029e010e224bec191d0f63ff695cdab04"
           "6abfd79dfbd00825839010ca1430dcf478c99b65e100d215e958894ea8d71"
           "d4ed63abc092e309409126e4d5448e3d990c79b4a83526fab1a9b2a303d91"
           "c1019d198681a000f4240",
           "8282582042c7b97f09cf640dcb76c7426c1181594dfc2da3aa000476aa9639"
           "bc0a131f4d0082583901687aa91847883af0c018dff89cbb837471a18cf61a"
           "5014182a63adb5a4045956f4152b201e6c534335a7248d62bf10400bfbbaa0b02dd"
           "3371a001e8480"}),
      CardanoCip30Serializer::SerializeUtxos(utxos));

  // Empty
  EXPECT_EQ(std::vector<std::string>({}),
            CardanoCip30Serializer::SerializeUtxos({}));
}

TEST(CardanoCip30SerializerTest, SerializeUtxos_Single) {
  cardano_rpc::UnspentOutputs utxos;
  cardano_rpc::UnspentOutput output;
  output.output_index = 0;
  output.lovelace_amount = 1000000u;
  std::vector<uint8_t> tx_bytes;
  base::HexStringToBytes(
      "d9ef8dcd983c6fe996d5029e010e224bec191d0f63ff695cdab046abfd79dfbd",
      &tx_bytes);
  base::SpanWriter(base::span(output.tx_hash)).Write(tx_bytes);
  output.address_to = *CardanoAddress::FromString(
      "addr1qyx2zscdearcexdktcgq6g27jkyff65dw82w6catczfwxz2qjy"
      "nwf42y3c7ejrrekj5r2fh6kx5m9gcrmywpqxw3np5qjeh38p");
  utxos.push_back(std::move(output));

  auto result = CardanoCip30Serializer::SerializeUtxos(utxos);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->size(), 1u);
}

TEST(CardanoCip30SerializerTest, DeserializeAmount_InvalidCBOR) {
  EXPECT_FALSE(CardanoCip30Serializer::DeserializeAmount("ZZZZ"));
  EXPECT_FALSE(CardanoCip30Serializer::DeserializeAmount("1a00"));
  EXPECT_FALSE(CardanoCip30Serializer::DeserializeAmount("6568656c6c6f"));
}

TEST(CardanoCip30SerializerTest, DeserializeAmount_Zero) {
  auto serialized = CardanoCip30Serializer::SerializeAmount(0u);
  EXPECT_TRUE(serialized.has_value());
  EXPECT_EQ(serialized.value(), "00");

  auto deserialized = CardanoCip30Serializer::DeserializeAmount(*serialized);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), 0u);
}

TEST(CardanoCip30SerializerTest, DeserializeAmount_SmallValues) {
  auto serialized_1ada = CardanoCip30Serializer::SerializeAmount(1000000u);
  EXPECT_TRUE(serialized_1ada.has_value());
  auto deserialized_1ada =
      CardanoCip30Serializer::DeserializeAmount(*serialized_1ada);
  EXPECT_TRUE(deserialized_1ada.has_value());
  EXPECT_EQ(deserialized_1ada.value(), 1000000u);

  auto serialized_min = CardanoCip30Serializer::SerializeAmount(1u);
  EXPECT_TRUE(serialized_min.has_value());
  auto deserialized_min =
      CardanoCip30Serializer::DeserializeAmount(*serialized_min);
  EXPECT_TRUE(deserialized_min.has_value());
  EXPECT_EQ(deserialized_min.value(), 1u);
}

TEST(CardanoCip30SerializerTest, SerializeAmount_LargeValues) {
  const uint64_t cardano_max_supply = 45000000000000000ULL;

  auto serialized = CardanoCip30Serializer::SerializeAmount(cardano_max_supply);
  EXPECT_TRUE(serialized.has_value());

  auto deserialized = CardanoCip30Serializer::DeserializeAmount(*serialized);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized.value(), cardano_max_supply);
}

TEST(CardanoCip30SerializerTest, SerializeAmount_UINT64_MAX_Fails) {
  auto serialized = CardanoCip30Serializer::SerializeAmount(UINT64_MAX);
  EXPECT_FALSE(serialized.has_value());
}

TEST(CardanoCip30SerializerTest, SerializeUtxos_EmptyReturnsEmptyArray) {
  cardano_rpc::UnspentOutputs empty_utxos;
  auto result = CardanoCip30Serializer::SerializeUtxos(empty_utxos);

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->empty());
}

TEST(CardanoCip30SerializerTest, Amount_RoundTrip) {
  std::vector<uint64_t> test_values = {
      0u,              // Zero
      1u,              // Minimum
      1000000u,        // 1 ADA
      1000000000u,     // 1000 ADA
      UINT64_MAX / 2,  // Maximum supported (from existing test)
  };

  for (uint64_t value : test_values) {
    auto serialized = CardanoCip30Serializer::SerializeAmount(value);
    ASSERT_TRUE(serialized.has_value()) << "Failed to serialize " << value;

    auto deserialized = CardanoCip30Serializer::DeserializeAmount(*serialized);
    ASSERT_TRUE(deserialized.has_value()) << "Failed to deserialize " << value;
    EXPECT_EQ(deserialized.value(), value)
        << "Round-trip mismatch for " << value;
  }
}

// Test that negative values (which convert to large uint64_t) are handled.
// Note: SerializeAmount takes uint64_t (unsigned), so -1 implicitly converts
// to UINT64_MAX, which should fail serialization (overflow protection).
TEST(CardanoCip30SerializerTest, SerializeAmount_NegativeValueConvertsToMax) {
  uint64_t negative_as_uint = static_cast<uint64_t>(-1);
  EXPECT_EQ(negative_as_uint, UINT64_MAX);

  auto serialized = CardanoCip30Serializer::SerializeAmount(negative_as_uint);
  EXPECT_FALSE(serialized.has_value())
      << "Negative value (converted to UINT64_MAX) should not serialize";
}

}  // namespace brave_wallet

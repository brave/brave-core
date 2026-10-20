// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/trust_keys.h"

#include <string>
#include <vector>

#include "base/strings/string_split.h"
#include "brave/components/brave_transfer/core/sync_code.h"
#include "brave/components/brave_transfer/core/test/test_vectors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

base::Time UnixTime(uint64_t seconds) {
  return base::Time::UnixEpoch() + base::Seconds(static_cast<int64_t>(seconds));
}

std::vector<std::string> TagStrings(const std::vector<Tag>& tags) {
  std::vector<std::string> result;
  for (const Tag& tag : tags) {
    result.push_back(test::HexString(tag));
  }
  return result;
}

ChainKeys KeysForByte(uint8_t value) {
  Key seed;
  seed.fill(value);
  return *ChainKeys::FromSeed(seed);
}

}  // namespace

TEST(BraveTransferChainKeysTest, KeyDerivationVectors) {
  base::DictValue vectors = test::LoadVectors("key_derivation.json");
  const base::ListValue* valid = vectors.FindList("valid");
  ASSERT_TRUE(valid);
  ASSERT_FALSE(valid->empty());
  for (const base::Value& entry : *valid) {
    const base::DictValue& v = entry.GetDict();
    SCOPED_TRACE(*v.FindString("name"));

    std::optional<Key> seed = SeedFromSyncCode(*v.FindString("sync_code"));
    ASSERT_TRUE(seed);
    EXPECT_EQ(test::HexString(*seed), *v.FindString("seed"));

    std::optional<ChainKeys> keys = ChainKeys::FromSeed(*seed);
    ASSERT_TRUE(keys);
    EXPECT_EQ(test::HexString(keys->root_key()), *v.FindString("root_key"));
    EXPECT_EQ(test::HexString(keys->psk()), *v.FindString("psk"));
    EXPECT_EQ(test::HexString(keys->discovery_key()),
              *v.FindString("discovery_key"));

    std::optional<ChainKeys> from_root =
        ChainKeys::FromRootKey(keys->root_key());
    ASSERT_TRUE(from_root);
    EXPECT_EQ(*from_root, *keys);
  }
}

TEST(BraveTransferChainKeysTest, InvalidSyncCodes) {
  base::DictValue vectors = test::LoadVectors("key_derivation.json");
  const base::ListValue* invalid = vectors.FindList("invalid_sync_codes");
  ASSERT_TRUE(invalid);
  ASSERT_FALSE(invalid->empty());
  for (const base::Value& entry : *invalid) {
    const base::DictValue& v = entry.GetDict();
    SCOPED_TRACE(*v.FindString("name"));
    EXPECT_FALSE(SeedFromSyncCode(*v.FindString("sync_code")));
  }
}

TEST(BraveTransferChainKeysTest, BadKeyLength) {
  std::vector<uint8_t> short_key(31);
  std::vector<uint8_t> long_key(33);
  EXPECT_FALSE(ChainKeys::FromSeed(short_key));
  EXPECT_FALSE(ChainKeys::FromSeed(long_key));
  EXPECT_FALSE(ChainKeys::FromRootKey(short_key));
  EXPECT_FALSE(ChainKeys::FromRootKey(long_key));
}

TEST(BraveTransferChainKeysTest, DiscoveryTagVectors) {
  base::DictValue vectors = test::LoadVectors("discovery_tags.json");
  std::optional<ChainKeys> keys =
      ChainKeys::FromSeed(test::FromHex(*vectors.FindString("seed")));
  ASSERT_TRUE(keys);
  EXPECT_EQ(test::HexString(keys->discovery_key()),
            *vectors.FindString("discovery_key"));

  for (const base::Value& entry : *vectors.FindList("tags")) {
    const base::DictValue& v = entry.GetDict();
    uint64_t unix_time = test::ToU64(v.Find("unix_time"));
    SCOPED_TRACE(unix_time);
    uint64_t epoch = test::ToU64(v.Find("epoch"));
    EXPECT_EQ(EpochForTime(UnixTime(unix_time)), epoch);
    EXPECT_EQ(test::HexString(keys->TagForEpoch(epoch)), *v.FindString("tag"));
  }

  for (const base::Value& entry : *vectors.FindList("accepted")) {
    const base::DictValue& v = entry.GetDict();
    uint64_t unix_time = test::ToU64(v.Find("unix_time"));
    SCOPED_TRACE(unix_time);
    std::vector<std::string> expected;
    for (const base::Value& tag : *v.FindList("tags")) {
      expected.push_back(tag.GetString());
    }
    std::vector<Tag> accepted = keys->AcceptedTags(UnixTime(unix_time));
    EXPECT_EQ(TagStrings(accepted), expected);
    for (const Tag& tag : accepted) {
      EXPECT_TRUE(keys->AcceptsTag(tag, UnixTime(unix_time)));
    }
  }

  const base::DictValue* txt = vectors.FindDict("txt_record");
  ASSERT_TRUE(txt);
  base::flat_map<std::string, std::string> expected;
  for (const base::Value& entry : *txt->FindList("entries")) {
    std::vector<std::string> parts = base::SplitString(
        entry.GetString(), "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    ASSERT_EQ(parts.size(), 2u);
    expected[parts[0]] = parts[1];
  }
  EXPECT_EQ(keys->TxtRecord(UnixTime(test::ToU64(txt->Find("unix_time")))),
            expected);
}

TEST(BraveTransferChainKeysTest, EpochBeforeUnixEpoch) {
  EXPECT_EQ(EpochForTime(base::Time::UnixEpoch() - base::Seconds(1)), 0u);
}

TEST(BraveTransferChainKeysTest, AdjacentEpochsOnly) {
  ChainKeys keys = KeysForByte(7);
  const base::Time now = UnixTime(1000 * kEpochSeconds);
  EXPECT_TRUE(keys.AcceptsTag(keys.TagForEpoch(999), now));
  EXPECT_TRUE(keys.AcceptsTag(keys.TagForEpoch(1000), now));
  EXPECT_TRUE(keys.AcceptsTag(keys.TagForEpoch(1001), now));
  EXPECT_FALSE(keys.AcceptsTag(keys.TagForEpoch(998), now));
  EXPECT_FALSE(keys.AcceptsTag(keys.TagForEpoch(1002), now));
  EXPECT_FALSE(KeysForByte(8).AcceptsTag(keys.TagForEpoch(1000), now));
  std::vector<uint8_t> short_tag(kTagSize - 1);
  EXPECT_FALSE(keys.AcceptsTag(short_tag, now));
}

TEST(BraveTransferChainKeysTest, TxtRecordMatching) {
  ChainKeys keys = KeysForByte(7);
  ChainKeys other = KeysForByte(8);
  const base::Time now = UnixTime(1792054400);
  const base::TimeDelta epoch = base::Seconds(kEpochSeconds);

  std::optional<Tag> match = keys.MatchTxtRecord(keys.TxtRecord(now), now);
  ASSERT_TRUE(match);
  EXPECT_EQ(*match, keys.TagForEpoch(EpochForTime(now)));
  EXPECT_TRUE(keys.MatchTxtRecord(keys.TxtRecord(now - epoch), now));
  EXPECT_FALSE(keys.MatchTxtRecord(keys.TxtRecord(now - 2 * epoch), now));
  EXPECT_FALSE(keys.MatchTxtRecord(other.TxtRecord(now), now));
  EXPECT_FALSE(keys.MatchTxtRecord(
      {{"v", "2"}, {"c", test::HexString(keys.TagForEpoch(0))}}, UnixTime(0)));
  EXPECT_FALSE(keys.MatchTxtRecord({{"v", "1"}, {"c", "zz"}}, now));
  EXPECT_FALSE(keys.MatchTxtRecord({{"v", "1"}}, now));
  EXPECT_FALSE(keys.MatchTxtRecord(
      {{"v", "1"}, {"c", test::HexString(keys.discovery_key())}}, now));
}

TEST(BraveTransferStaticKeyTest, FingerprintVectors) {
  base::DictValue vectors = test::LoadVectors("noise_handshake.json");
  for (const char* side : {"initiator", "responder"}) {
    SCOPED_TRACE(side);
    std::string prefix(side);
    Key private_key;
    base::span(private_key)
        .copy_from(test::FromHex(*vectors.FindString(prefix + "_static")));
    Key public_key = static_key::PublicKey(private_key);
    EXPECT_EQ(test::HexString(public_key),
              *vectors.FindString(prefix + "_static_public"));
    EXPECT_EQ(static_key::Fingerprint(public_key),
              *vectors.FindString(prefix + "_fingerprint"));
  }
}

TEST(BraveTransferStaticKeyTest, GeneratedKeysDiffer) {
  Key a = static_key::GeneratePrivateKey();
  Key b = static_key::GeneratePrivateKey();
  EXPECT_NE(a, b);
  EXPECT_NE(static_key::PublicKey(a), static_key::PublicKey(b));
}

}  // namespace brave_transfer

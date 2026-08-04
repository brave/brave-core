/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_utils.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/to_value_list.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {
namespace {

constexpr char kValidBridge[] =
    "obfs4 193.11.166.194:27020 86AC7B8D430DAC4117E9F42C9EAED18133863AAF "
    "cert=0LDeJH4JzMDtkJJrFphJCiPqKx7loozKN7VNfuukMGfHO0Z8OGdzHVkhVAOfo1mUdv9c"
    "Mg iat-mode=0";
constexpr char kAnotherValidBridge[] =
    "obfs4 85.31.186.26:443 91A6354697E6B02A386312F68D82CF86824D3606 "
    "cert=PBwr+S8JTVZo6MPdHnkTwXJPILWADLqfMGoVvhZClMq/Urndyd42BwX9YFJHZnBB3H0X"
    "Cw iat-mode=0";

// Lines that must survive validation. A bridge line is
// `[transport] host[:port] [fingerprint] [key=value ...]`, so every prefix of
// that grammar has to be accepted.
constexpr const char* kWellFormedBridges[] = {
    // The minimal form the settings UI asks users for ("Type address:port").
    "192.0.2.1:443",
    // Bare host, no port: Tor defaults it to 443.
    "192.0.2.1",
    "example.com:443",
    // IPv6 literals are bracketed.
    "[2001:db8::1]:443",
    "[2001:db8::1]",
    // Transport plus address, no fingerprint.
    "obfs4 192.0.2.1:443",
    // Transport, address and fingerprint, no arguments.
    "obfs4 192.0.2.1:443 86AC7B8D430DAC4117E9F42C9EAED18133863AAF",
    // Fingerprints are hex in either case, and may be mixed.
    "obfs4 192.0.2.1:443 86ac7b8d430dac4117e9f42c9eaed18133863aaf",
    "obfs4 192.0.2.1:443 86Ac7B8d430DaC4117e9F42c9EaEd18133863aAf",
    // Address plus arguments, no transport and no fingerprint.
    "192.0.2.1:443 iat-mode=0",
    // Underscores and dashes appear in real transport names and argument keys.
    ("meek_lite 192.0.2.18:80 url=https://meek.azureedge.net/ "
     "front=ajax.aspnetcdn.com"),
    "snowflake 192.0.2.4:80 utls-imitate=hellorandomizedalpn",
    // Hosts may contain underscores and be upper-case.
    "obfs4 My_Bridge.Example.COM:443",
    // An '=' inside a value is opaque to us and must be preserved.
    "obfs4 192.0.2.1:443 cert=abc=def==",
    kValidBridge,
    kAnotherValidBridge,
};

// Control-character and quoting attacks against the Tor control port command
// these lines are interpolated into.
constexpr const char* kUnsafeCharacterBridges[] = {
    // Closes the quoted argument.
    "obfs4 192.0.2.1:443 iat-mode=0\" UseBridges=0",
    // Escapes the closing quote.
    "obfs4 192.0.2.1:443 iat-mode=0\\",
    // A backslash anywhere in the line.
    "obfs4 192.0.2.1:443 cert=a\\b",
    // Injects a second control command.
    "obfs4 192.0.2.1:443\r\nSETCONF SocksPort=1234",
    "obfs4 192.0.2.1:443\nSETCONF SocksPort=1234",
    "obfs4 192.0.2.1:443\rSETCONF SocksPort=1234",
    // Leading injection.
    "\r\nSETEVENTS",
    // Bare quote.
    "\"",
    // A tab is not a field separator for Tor, so it stays inside a token.
    "obfs4\t192.0.2.1:443",
    // Non-ASCII: a Cyrillic 'о' homoglyph in the transport name.
    "оbfs4 192.0.2.1:443",
};

// Lines that are free of dangerous characters but do not match the grammar.
constexpr const char* kMalformedBridges[] = {
    "",
    " ",
    // No address at all.
    "obfs4",
    // Transport name with an illegal character.
    "obfs$4 192.0.2.1:443",
    // Not a host:port.
    "obfs4 :443",
    "obfs4 192.0.2.1:",
    "obfs4 192.0.2.1:notaport",
    "obfs4 192.0.2.1:99999",
    "obfs4 192.0.2.1:0",
    "obfs4 192.0.2.1:443:443",
    // Credentials embedded in the authority.
    "obfs4 user:password@192.0.2.1:443",
    // Unbracketed IPv6 literal: the trailing group parses as a bogus port.
    "obfs4 2001:db8::1",
    // Illegal characters in the host.
    "obfs4 192.0.2.1/../:443",
    "obfs4 exa mple.com:443",
    // A trailing field that is neither a fingerprint nor a key=value pair. A
    // 39-hex-digit token is the near-miss case.
    "obfs4 192.0.2.1:443 86AC7B8D430DAC4117E9F42C9EAED18133863AA",
    "obfs4 192.0.2.1:443 notakeyvaluepair",
    // Fingerprint-length token that is not hex.
    "obfs4 192.0.2.1:443 ZZAC7B8D430DAC4117E9F42C9EAED18133863AAF",
    // Malformed key=value pairs.
    "obfs4 192.0.2.1:443 =0",
    "obfs4 192.0.2.1:443 iat-mode=",
    "obfs4 192.0.2.1:443 iat mode=0",
    "obfs4 192.0.2.1:443 iat$mode=0",
    // Only one fingerprint may precede the arguments.
    ("obfs4 192.0.2.1:443 86AC7B8D430DAC4117E9F42C9EAED18133863AAF "
     "91A6354697E6B02A386312F68D82CF86824D3606"),
};

// An entry that is rejected, used where the reason does not matter.
constexpr char kInvalidBridge[] = "obfs4 192.0.2.1:443 notakeyvaluepair";

base::ListValue MakeList(const std::vector<std::string>& items) {
  return base::ToValueList(items);
}

}  // namespace

// -- Grammar -----------------------------------------------------------------

TEST(TorUtilsTest, WellFormedBridgesAreAccepted) {
  for (const char* bridge : kWellFormedBridges) {
    EXPECT_TRUE(IsValidBridgeLine(bridge)) << "wrongly rejected: " << bridge;
  }
}

TEST(TorUtilsTest, BridgesWithUnsafeCharactersAreRejected) {
  for (const char* bridge : kUnsafeCharacterBridges) {
    EXPECT_FALSE(IsValidBridgeLine(bridge)) << "wrongly accepted: " << bridge;
  }
}

TEST(TorUtilsTest, MalformedBridgesAreRejected) {
  for (const char* bridge : kMalformedBridges) {
    EXPECT_FALSE(IsValidBridgeLine(bridge)) << "wrongly accepted: " << bridge;
  }
}

TEST(TorUtilsTest, EmbeddedNulIsRejected) {
  // Without the NUL this line is well formed, so the NUL is what fails it.
  EXPECT_FALSE(IsValidBridgeLine(
      std::string_view("obfs4 192.0.2.1:443\0 iat-mode=0", 31)));
}

TEST(TorUtilsTest, OverlongBridgesAreRejected) {
  // A single argument long enough to push an otherwise well formed line past
  // the length cap on its own.
  const std::string too_long = base::StrCat(
      {"obfs4 192.0.2.1:443 pad=", std::string(kMaxBridgeLineLength, 'a')});
  EXPECT_FALSE(IsValidBridgeLine(too_long));

  // The same line truncated to exactly the cap is still accepted.
  EXPECT_TRUE(IsValidBridgeLine(too_long.substr(0, kMaxBridgeLineLength)));
}

TEST(TorUtilsTest, HardcodedBuiltinBridgesPassValidation) {
  // The hardcoded fallback lists are the reference for what a well formed
  // bridge line looks like, so validation must never reject one of them. This
  // guards against a future built-in bridge being silently dropped.
  for (auto type : {BridgesConfig::BuiltinType::kSnowflake,
                    BridgesConfig::BuiltinType::kObfs4,
                    BridgesConfig::BuiltinType::kMeekAzure}) {
    SCOPED_TRACE(static_cast<int>(type));
    BridgesConfig config;
    config.use_builtin = type;
    const auto& hardcoded = config.GetBuiltinBridges();
    ASSERT_FALSE(hardcoded.empty());
    for (const auto& bridge : hardcoded) {
      EXPECT_TRUE(IsValidBridgeLine(bridge))
          << "built-in bridge rejected: " << bridge;
    }
  }
}

// -- Filtering ---------------------------------------------------------------

TEST(TorUtilsTest, FilterBridgeLinesDropsInvalidEntriesInPlace) {
  // Valid entries around an invalid one are kept, in order.
  const std::vector<std::string> input = {kValidBridge, kInvalidBridge,
                                          kAnotherValidBridge};
  EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
            FilterBridgeLines(input));
}

TEST(TorUtilsTest, FilterBridgeLinesCapsTheList) {
  // Ports differ so that the entries are distinguishable and order is checked.
  std::vector<std::string> input;
  for (size_t i = 0; i < kMaxBridgeLines * 2; ++i) {
    input.push_back(
        base::StrCat({"obfs4 192.0.2.1:", base::NumberToString(443 + i)}));
  }
  EXPECT_EQ(
      std::vector<std::string>(input.begin(), input.begin() + kMaxBridgeLines),
      FilterBridgeLines(input));
}

TEST(TorUtilsTest, FilterBridgeLinesCountsOnlyValidEntriesTowardsTheCap) {
  // An invalid entry must not consume one of the kMaxBridgeLines slots.
  std::vector<std::string> input;
  for (size_t i = 0; i < kMaxBridgeLines; ++i) {
    input.push_back(
        base::StrCat({"obfs4 192.0.2.1:", base::NumberToString(443 + i)}));
    input.push_back(kInvalidBridge);
  }
  EXPECT_EQ(kMaxBridgeLines, FilterBridgeLines(input).size());
}

TEST(TorUtilsTest, FilterBridgeLinesHandlesEmptyResult) {
  const std::vector<std::string> empty;
  EXPECT_TRUE(FilterBridgeLines(empty).empty());

  const std::vector<std::string> all_invalid = {kInvalidBridge};
  EXPECT_TRUE(FilterBridgeLines(all_invalid).empty());
}

// -- Pref loading ------------------------------------------------------------

TEST(TorUtilsTest, SubmittedBridgeListsAreLoadedVerbatim) {
  // The settings page writes back whatever it reads, so filtering here would
  // erase a rejected line from the user's settings instead of ignoring it.
  // BraveTorHandler::SetBridgesConfig() refuses to store one in the first
  // place, and TorControl::SetupBridges() drops it at the point of use.
  const std::vector<std::string> input = {kValidBridge, kInvalidBridge,
                                          kAnotherValidBridge};
  base::DictValue dict;
  dict.Set("provided_bridges", MakeList(input));
  dict.Set("requested_bridges", MakeList(input));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);
  EXPECT_EQ(input, config->provided_bridges);
  EXPECT_EQ(input, config->requested_bridges);
}

TEST(TorUtilsTest, BuiltinBridgesAreFiltered) {
  // builtin_bridges arrives from Tor's moat service, with nobody to report a
  // bad line to, so it is filtered as it is ingested.
  const std::vector<std::string> input = {
      kValidBridge, "obfs4 192.0.2.1:443\r\nSETCONF SocksPort=1234",
      kInvalidBridge, kAnotherValidBridge};

  base::DictValue builtin;
  builtin.Set("obfs4", MakeList(input));

  base::DictValue dict;
  dict.Set("use_builtin_bridges",
           static_cast<int>(BridgesConfig::BuiltinType::kObfs4));
  dict.Set("builtin_bridges", std::move(builtin));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);
  EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
            config->GetBuiltinBridges());
}

TEST(TorUtilsTest, BuiltinBridgesFallBackWhenAllEntriesInvalid) {
  base::DictValue builtin;
  builtin.Set("obfs4", MakeList({"obfs4 192.0.2.1:443 \"",
                                 "obfs4 192.0.2.1:443\r\nSETEVENTS"}));

  base::DictValue dict;
  dict.Set("use_builtin_bridges",
           static_cast<int>(BridgesConfig::BuiltinType::kObfs4));
  dict.Set("builtin_bridges", std::move(builtin));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);
  // Nothing was stored for obfs4, so the hardcoded list is used and none of
  // the injected entries are visible.
  EXPECT_FALSE(
      config->builtin_bridges.contains(BridgesConfig::BuiltinType::kObfs4));
  EXPECT_FALSE(config->GetBuiltinBridges().empty());
}

TEST(TorUtilsTest, NonStringEntriesAreSkipped) {
  base::ListValue list;
  list.Append(kValidBridge);
  list.Append(42);
  list.Append(true);
  list.Append(base::ListValue());
  list.Append(base::DictValue());
  list.Append(kAnotherValidBridge);

  base::DictValue dict;
  dict.Set("provided_bridges", std::move(list));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);
  EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
            config->provided_bridges);
}

TEST(TorUtilsTest, AllFieldsSurviveRoundTrip) {
  base::DictValue builtin;
  builtin.Set("snowflake", MakeList({kValidBridge}));

  base::DictValue dict;
  dict.Set("use_bridges", static_cast<int>(BridgesConfig::Usage::kProvide));
  dict.Set("use_builtin_bridges",
           static_cast<int>(BridgesConfig::BuiltinType::kSnowflake));
  dict.Set("builtin_bridges", std::move(builtin));
  dict.Set("provided_bridges", MakeList({kValidBridge, kAnotherValidBridge}));
  dict.Set("requested_bridges", MakeList({kAnotherValidBridge}));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);

  auto reparsed = BridgesConfig::FromDict(config->ToDict());
  ASSERT_TRUE(reparsed);
  EXPECT_EQ(BridgesConfig::Usage::kProvide, reparsed->use_bridges);
  EXPECT_EQ(BridgesConfig::BuiltinType::kSnowflake, reparsed->use_builtin);
  EXPECT_EQ(std::vector<std::string>({kValidBridge}),
            reparsed->GetBuiltinBridges());
  EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
            reparsed->provided_bridges);
  EXPECT_EQ(std::vector<std::string>({kAnotherValidBridge}),
            reparsed->requested_bridges);
}

}  // namespace tor

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_utils.h"

#include <string>
#include <string_view>
#include <vector>

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
    // Lower-case and mixed-case fingerprints are both hex.
    "obfs4 192.0.2.1:443 86ac7b8d430dac4117e9f42c9eaed18133863aaf",
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

// Control-character and quoting attacks. Bridge lines are interpolated into a
// double-quoted argument of a Tor control port SETCONF command, so each of
// these would either break out of the quoting or inject an extra command.
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

base::ListValue MakeList(const std::vector<std::string>& items) {
  base::ListValue list;
  for (const auto& item : items) {
    list.Append(item);
  }
  return list;
}

// Round-trips `bridges` through the pref representation and returns the list as
// BridgesConfig parsed it.
std::vector<std::string> LoadProvidedBridges(
    const std::vector<std::string>& bridges) {
  base::DictValue dict;
  dict.Set("provided_bridges", MakeList(bridges));
  auto config = BridgesConfig::FromDict(dict);
  if (!config) {
    return {};
  }
  return std::move(config->provided_bridges);
}

std::vector<std::string> LoadRequestedBridges(
    const std::vector<std::string>& bridges) {
  base::DictValue dict;
  dict.Set("requested_bridges", MakeList(bridges));
  auto config = BridgesConfig::FromDict(dict);
  if (!config) {
    return {};
  }
  return std::move(config->requested_bridges);
}

std::vector<std::string> LoadBuiltinBridges(
    const std::vector<std::string>& bridges) {
  base::DictValue builtin;
  builtin.Set("obfs4", MakeList(bridges));

  base::DictValue dict;
  dict.Set("use_builtin_bridges",
           static_cast<int>(BridgesConfig::BuiltinType::kObfs4));
  dict.Set("builtin_bridges", std::move(builtin));

  auto config = BridgesConfig::FromDict(dict);
  if (!config) {
    return {};
  }
  auto found = config->builtin_bridges.find(BridgesConfig::BuiltinType::kObfs4);
  if (found == config->builtin_bridges.end()) {
    return {};
  }
  return found->second;
}

}  // namespace

TEST(TorUtilsTest, WellFormedBridgesAreAccepted) {
  for (const char* bridge : kWellFormedBridges) {
    EXPECT_EQ(std::vector<std::string>({bridge}), LoadProvidedBridges({bridge}))
        << "wrongly rejected: " << bridge;
  }
}

TEST(TorUtilsTest, BridgesWithUnsafeCharactersAreSkipped) {
  for (const char* bridge : kUnsafeCharacterBridges) {
    EXPECT_EQ(std::vector<std::string>(), LoadProvidedBridges({bridge}))
        << "wrongly accepted: " << bridge;
  }
}

TEST(TorUtilsTest, MalformedBridgesAreSkipped) {
  for (const char* bridge : kMalformedBridges) {
    EXPECT_EQ(std::vector<std::string>(), LoadProvidedBridges({bridge}))
        << "wrongly accepted: " << bridge;
  }
}

TEST(TorUtilsTest, InvalidBridgesAreSkippedInPlace) {
  // Valid entries around an invalid one are kept, in order.
  for (const char* bridge : kUnsafeCharacterBridges) {
    EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
              LoadProvidedBridges({kValidBridge, bridge, kAnotherValidBridge}))
        << "not skipped: " << bridge;
  }
  for (const char* bridge : kMalformedBridges) {
    EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
              LoadProvidedBridges({kValidBridge, bridge, kAnotherValidBridge}))
        << "not skipped: " << bridge;
  }
}

TEST(TorUtilsTest, ValidationAppliesToEveryBridgeList) {
  // provided_bridges comes from the user, requested_bridges from BridgeDB and
  // builtin_bridges from the component updater. All three go through the same
  // validation.
  const std::vector<std::string> input = {
      kValidBridge, "obfs4 192.0.2.1:443\r\nSETCONF SocksPort=1234",
      "obfs4 192.0.2.1:443 notakeyvaluepair", kAnotherValidBridge};
  const std::vector<std::string> expected = {kValidBridge, kAnotherValidBridge};

  EXPECT_EQ(expected, LoadProvidedBridges(input));
  EXPECT_EQ(expected, LoadRequestedBridges(input));
  EXPECT_EQ(expected, LoadBuiltinBridges(input));
}

TEST(TorUtilsTest, HardcodedBuiltinBridgesPassValidation) {
  // The hardcoded fallback lists are the reference for what a well formed
  // bridge line looks like, so validation must never reject one of them. This
  // guards against a future built-in bridge being silently dropped.
  for (auto type : {BridgesConfig::BuiltinType::kSnowflake,
                    BridgesConfig::BuiltinType::kObfs4,
                    BridgesConfig::BuiltinType::kMeekAzure}) {
    BridgesConfig config;
    config.use_builtin = type;
    const auto& hardcoded = config.GetBuiltinBridges();
    ASSERT_FALSE(hardcoded.empty());
    for (const auto& bridge : hardcoded) {
      EXPECT_EQ(std::vector<std::string>({bridge}),
                LoadProvidedBridges({bridge}))
          << "built-in bridge rejected: " << bridge;
    }
  }
}

TEST(TorUtilsTest, EmbeddedNulIsSkipped) {
  base::ListValue list;
  list.Append(std::string("obfs4 192.0.2.1:443\0 iat-mode=0", 31));

  base::DictValue dict;
  dict.Set("provided_bridges", std::move(list));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);
  EXPECT_TRUE(config->provided_bridges.empty());
}

TEST(TorUtilsTest, OverlongBridgesAreSkipped) {
  // 1024 characters is the cap. Grow a valid line with long-but-well-formed
  // arguments, so that the length cap is what rejects it rather than the cap
  // on the number of fields.
  const std::string padding =
      base::StrCat({" padding-key=", std::string(240, 'a')});
  std::string bridge = "obfs4 192.0.2.1:443";
  std::string last_in_bounds;
  while (bridge.size() <= 1024) {
    last_in_bounds = bridge;
    base::StrAppend(&bridge, {padding});
  }
  ASSERT_GT(bridge.size(), 1024u);
  EXPECT_EQ(std::vector<std::string>(), LoadProvidedBridges({bridge}));

  // A long but in-bounds line is still accepted.
  ASSERT_LE(last_in_bounds.size(), 1024u);
  ASSERT_GT(last_in_bounds.size(), 768u);
  EXPECT_EQ(std::vector<std::string>({last_in_bounds}),
            LoadProvidedBridges({last_in_bounds}));
}

TEST(TorUtilsTest, BridgeListIsCapped) {
  // Bounds the size of the SETCONF command built from the list.
  std::vector<std::string> input;
  for (int i = 0; i < 100; ++i) {
    input.push_back(
        base::StrCat({"obfs4 192.0.2.1:", base::NumberToString(1024 + i)}));
  }
  const auto loaded = LoadProvidedBridges(input);
  EXPECT_EQ(64u, loaded.size());
  // The cap keeps the first entries, in order.
  EXPECT_EQ(input[0], loaded.front());
  EXPECT_EQ(input[63], loaded.back());
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

TEST(TorUtilsTest, ValidBridgesSurviveRoundTrip) {
  base::DictValue dict;
  dict.Set("provided_bridges", MakeList({kValidBridge, kAnotherValidBridge}));
  dict.Set("requested_bridges", MakeList({kAnotherValidBridge}));

  auto config = BridgesConfig::FromDict(dict);
  ASSERT_TRUE(config);

  auto reparsed = BridgesConfig::FromDict(config->ToDict());
  ASSERT_TRUE(reparsed);
  EXPECT_EQ(std::vector<std::string>({kValidBridge, kAnotherValidBridge}),
            reparsed->provided_bridges);
  EXPECT_EQ(std::vector<std::string>({kAnotherValidBridge}),
            reparsed->requested_bridges);
}

}  // namespace tor

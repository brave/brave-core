/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_utils.h"

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/components/tor/tor_constants.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {
constexpr const char kUseBridgesKey[] = "use_bridges";
constexpr const char kUseBuiltinBridgesKey[] = "use_builtin_bridges";
constexpr const char kRequestedBrigesKey[] = "requested_bridges";
constexpr const char kProvidedBridgesKey[] = "provided_bridges";

template <typename Enum>
struct MinMaxTraits;

template <>
struct MinMaxTraits<tor::BridgesConfig::Usage> {
  static constexpr tor::BridgesConfig::Usage kMin =
      tor::BridgesConfig::Usage::kNotUsed;
  static constexpr tor::BridgesConfig::Usage kMax =
      tor::BridgesConfig::Usage::kProvide;
};

template <>
struct MinMaxTraits<tor::BridgesConfig::BuiltinType> {
  static constexpr tor::BridgesConfig::BuiltinType kMin =
      tor::BridgesConfig::BuiltinType::kSnowflake;
  static constexpr tor::BridgesConfig::BuiltinType kMax =
      tor::BridgesConfig::BuiltinType::kMeekAzure;
};

template <typename Enum>
Enum CastToEnum(const int value) {
  if (value >= static_cast<int>(MinMaxTraits<Enum>::kMin) &&
      value <= static_cast<int>(MinMaxTraits<Enum>::kMax)) {
    return static_cast<Enum>(value);
  }
  return MinMaxTraits<Enum>::kMin;
}

}  // namespace

namespace tor {

BridgesConfig::BridgesConfig() = default;
BridgesConfig::BridgesConfig(BridgesConfig&&) noexcept = default;
BridgesConfig::~BridgesConfig() = default;

BridgesConfig& BridgesConfig::operator=(BridgesConfig&&) noexcept = default;

// clang-format off
const std::vector<std::string>& BridgesConfig::GetBuiltinBridges() const {
  static const base::NoDestructor<std::vector<std::string>> kSnowflakeBridges(
      {"snowflake 192.0.2.3:1 2B280B23E1107BB62ABFC40DDCC8824814F80A72"});

  static const base::NoDestructor<std::vector<std::string>> kObfs4Briges({
      "obfs4 45.145.95.6:27015 C5B7CD6946FF10C5B3E89691A7D3F2C122D2117C cert=TD7PbUO0/0k6xYHMPW3vJxICfkMZNdkRrb63Zhl5j9dW3iRGiCx0A7mPhe5T2EDzQ35+Zw iat-mode=0",  // NOLINT
      "obfs4 51.222.13.177:80 5EDAC3B810E12B01F6FD8050D2FD3E277B289A08 cert=2uplIpLQ0q9+0qMFrK5pkaYRDOe460LL9WHBvatgkuRr/SL31wBOEupaMMJ6koRE6Ld0ew iat-mode=0",  // NOLINT
      "obfs4 193.11.166.194:27025 1AE2C08904527FEA90C4C4F8C1083EA59FBC6FAF cert=ItvYZzW5tn6v3G4UnQa6Qz04Npro6e81AP70YujmK/KXwDFPTs3aHXcHp4n8Vt6w/bv8cA iat-mode=0",  // NOLINT
      "obfs4 146.57.248.225:22 10A6CD36A537FCE513A322361547444B393989F0 cert=K1gDtDAIcUfeLqbstggjIw2rtgIKqdIhUlHp82XRqNSq/mtAjp1BIC9vHKJ2FAEpGssTPw iat-mode=0",  // NOLINT
      "obfs4 38.229.33.83:80 0BAC39417268B96B9F514E7F63FA6FBA1A788955 cert=VwEFpk9F/UN9JED7XpG1XOjm/O8ZCXK80oPecgWnNDZDv5pdkhq1OpbAH0wNqOT6H6BmRQ iat-mode=1",  // NOLINT
      "obfs4 193.11.166.194:27020 86AC7B8D430DAC4117E9F42C9EAED18133863AAF cert=0LDeJH4JzMDtkJJrFphJCiPqKx7loozKN7VNfuukMGfHO0Z8OGdzHVkhVAOfo1mUdv9cMg iat-mode=0",  // NOLINT
      "obfs4 [2a0c:4d80:42:702::1]:27015 C5B7CD6946FF10C5B3E89691A7D3F2C122D2117C cert=TD7PbUO0/0k6xYHMPW3vJxICfkMZNdkRrb63Zhl5j9dW3iRGiCx0A7mPhe5T2EDzQ35+Zw iat-mode=0",  // NOLINT
      "obfs4 192.95.36.142:443 CDF2E852BF539B82BD10E27E9115A31734E378C2 cert=qUVQ0srL1JI/vO6V6m/24anYXiJD3QP2HgzUKQtQ7GRqqUvs7P+tG43RtAqdhLOALP7DJQ iat-mode=1",  // NOLINT
      "obfs4 193.11.166.194:27015 2D82C2E354D531A68469ADF7F878FA6060C6BACA cert=4TLQPJrTSaDffMK7Nbao6LC7G9OW/NHkUwIdjLSS3KYf0Nv4/nQiiI8dY2TcsQx01NniOg iat-mode=0",  // NOLINT
      "obfs4 38.229.1.78:80 C8CBDB2464FC9804A69531437BCF2BE31FDD2EE4 cert=Hmyfd2ev46gGY7NoVxA9ngrPF2zCZtzskRTzoWXbxNkzeVnGFPWmrTtILRyqCTjHR+s9dg iat-mode=1",  // NOLINT
      "obfs4 209.148.46.65:443 74FAD13168806246602538555B5521A0383A1875 cert=ssH+9rP8dG2NLDN2XuFw63hIO/9MNNinLmxQDpVa+7kTOa9/m+tGWT1SmSYpQ9uTBGa6Hw iat-mode=0",  // NOLINT
      "obfs4 144.217.20.138:80 FB70B257C162BF1038CA669D568D76F5B7F0BABB cert=vYIV5MgrghGQvZPIi1tJwnzorMgqgmlKaB77Y3Z9Q/v94wZBOAXkW+fdx4aSxLVnKO+xNw iat-mode=0",  // NOLINT
      "obfs4 85.31.186.26:443 91A6354697E6B02A386312F68D82CF86824D3606 cert=PBwr+S8JTVZo6MPdHnkTwXJPILWADLqfMGoVvhZClMq/Urndyd42BwX9YFJHZnBB3H0XCw iat-mode=0",  // NOLINT
      "obfs4 85.31.186.98:443 011F2599C0E9B27EE74B353155E244813763C3E5 cert=ayq0XzCwhpdysn5o0EyDUbmSOx3X/oTEbzDMvczHOdBJKlvIdHHLJGkZARtT4dcBFArPPg iat-mode=0",  // NOLINT
      "obfs4 37.218.245.14:38224 D9A82D2F9C2F65A18407B1D2B764F130847F8B5D cert=bjRaMrr1BRiAW8IE9U5z27fQaYgOhX1UCmOpg2pFpoMvo6ZgQMzLsaTzzQNTlm7hNcb+Sg iat-mode=0"  // NOLINT
  });

  static const base::NoDestructor<std::vector<std::string>> kMeekAzureBriges({
      "meek_lite 192.0.2.2:2 97700DFE9F483596DDA6264C4D7DF7641E1E39CE "
      "url=https://meek.azureedge.net/ front=ajax.aspnetcdn.com"});

  switch (use_builtin) {
    case BuiltinType::kSnowflake:
      return *kSnowflakeBridges;
    case BuiltinType::kObfs4:
      return *kObfs4Briges;
    case BuiltinType::kMeekAzure:
      return *kMeekAzureBriges;
  }
}
// clang-format on

// static
absl::optional<BridgesConfig> BridgesConfig::FromDict(
    const base::Value::Dict& dict) {
  BridgesConfig result;
  result.use_bridges =
      CastToEnum<Usage>(dict.FindInt(kUseBridgesKey).value_or(0));

  result.use_builtin =
      CastToEnum<BuiltinType>(dict.FindInt(kUseBuiltinBridgesKey).value_or(1));

  if (auto* bridges = dict.FindList(kProvidedBridgesKey)) {
    for (const auto& bridge : *bridges) {
      if (!bridge.is_string())
        continue;
      result.provided_bridges.push_back(bridge.GetString());
    }
  }
  if (auto* requested_bridges = dict.FindList(kRequestedBrigesKey)) {
    for (const auto& bridge : *requested_bridges) {
      if (!bridge.is_string())
        continue;
      result.requested_bridges.push_back((bridge.GetString()));
    }
  }

  return result;
}

// static
absl::optional<BridgesConfig> BridgesConfig::FromValue(const base::Value* v) {
  if (!v || !v->is_dict())
    return absl::nullopt;
  return FromDict(v->GetDict());
}

base::Value::Dict BridgesConfig::ToDict() const {
  base::Value::Dict result;
  result.Set(kUseBridgesKey, static_cast<int>(use_bridges));
  result.Set(kUseBuiltinBridgesKey, static_cast<int>(use_builtin));

  base::Value::List list;
  for (const auto& bridge : provided_bridges) {
    list.Append(bridge);
  }
  result.Set(kProvidedBridgesKey, std::move(list));

  list = base::Value::List();
  for (const auto& bridge : requested_bridges) {
    list.Append(bridge);
  }
  result.Set(kRequestedBrigesKey, std::move(list));

  return result;
}

base::Value BridgesConfig::ToValue() const {
  return base::Value(ToDict());
}

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state) {
  // Do this for legacy tor profile migration because tor profile might be last
  // active profile before upgrading
  std::string last_used_profile_name =
      local_state->GetString(prefs::kProfileLastUsed);
  if (!last_used_profile_name.empty() &&
      last_used_profile_name ==
          base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe()) {
    local_state->SetString(prefs::kProfileLastUsed, chrome::kInitialProfile);
  }
}

}  // namespace tor

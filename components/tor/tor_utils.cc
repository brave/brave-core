/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_utils.h"

#include <optional>
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

namespace {
constexpr const char kUseBridgesKey[] = "use_bridges";
constexpr const char kUseBuiltinBridgesKey[] = "use_builtin_bridges";
constexpr const char kBuiltinBridgesKey[] = "builtin_bridges";
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

const char* GetBuiltinTypeName(tor::BridgesConfig::BuiltinType type) {
  switch (type) {
    case tor::BridgesConfig::BuiltinType::kSnowflake:
      return "snowflake";
    case tor::BridgesConfig::BuiltinType::kObfs4:
      return "obfs4";
    case tor::BridgesConfig::BuiltinType::kMeekAzure:
      return "meek-azure";
  }
}

const std::vector<std::string>& GetBuiltinBridges(
    tor::BridgesConfig::BuiltinType type) {
  // clang-format off
  static const base::NoDestructor<std::vector<std::string>> kSnowflakeBridges({
    "snowflake 192.0.2.4:80 8838024498816A039FCBBAB14E6F40A0843051FA fingerprint=8838024498816A039FCBBAB14E6F40A0843051FA url=https://1098762253.rsc.cdn77.org/ fronts=www.cdn77.com,www.phpmyadmin.net ice=stun:stun.l.google.com:19302,stun:stun.antisip.com:3478,stun:stun.bluesip.net:3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel.net:3478,stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:3478 utls-imitate=hellorandomizedalpn",  // NOLINT
    "snowflake 192.0.2.3:80 2B280B23E1107BB62ABFC40DDCC8824814F80A72 fingerprint=2B280B23E1107BB62ABFC40DDCC8824814F80A72 url=https://1098762253.rsc.cdn77.org/ fronts=www.cdn77.com,www.phpmyadmin.net ice=stun:stun.l.google.com:19302,stun:stun.antisip.com:3478,stun:stun.bluesip.net:3478,stun:stun.dus.net:3478,stun:stun.epygi.com:3478,stun:stun.sonetel.com:3478,stun:stun.uls.co.za:3478,stun:stun.voipgate.com:3478,stun:stun.voys.nl:3478 utls-imitate=hellorandomizedalpn"  // NOLINT
  });

  static const base::NoDestructor<std::vector<std::string>> kObfs4Briges({
    "obfs4 193.11.166.194:27020 86AC7B8D430DAC4117E9F42C9EAED18133863AAF cert=0LDeJH4JzMDtkJJrFphJCiPqKx7loozKN7VNfuukMGfHO0Z8OGdzHVkhVAOfo1mUdv9cMg iat-mode=0",  // NOLINT
    "obfs4 193.11.166.194:27015 2D82C2E354D531A68469ADF7F878FA6060C6BACA cert=4TLQPJrTSaDffMK7Nbao6LC7G9OW/NHkUwIdjLSS3KYf0Nv4/nQiiI8dY2TcsQx01NniOg iat-mode=0",  // NOLINT
    "obfs4 85.31.186.26:443 91A6354697E6B02A386312F68D82CF86824D3606 cert=PBwr+S8JTVZo6MPdHnkTwXJPILWADLqfMGoVvhZClMq/Urndyd42BwX9YFJHZnBB3H0XCw iat-mode=0",  // NOLINT
    "obfs4 146.57.248.225:22 10A6CD36A537FCE513A322361547444B393989F0 cert=K1gDtDAIcUfeLqbstggjIw2rtgIKqdIhUlHp82XRqNSq/mtAjp1BIC9vHKJ2FAEpGssTPw iat-mode=0",  // NOLINT
    "obfs4 209.148.46.65:443 74FAD13168806246602538555B5521A0383A1875 cert=ssH+9rP8dG2NLDN2XuFw63hIO/9MNNinLmxQDpVa+7kTOa9/m+tGWT1SmSYpQ9uTBGa6Hw iat-mode=0",  // NOLINT
    "obfs4 193.11.166.194:27025 1AE2C08904527FEA90C4C4F8C1083EA59FBC6FAF cert=ItvYZzW5tn6v3G4UnQa6Qz04Npro6e81AP70YujmK/KXwDFPTs3aHXcHp4n8Vt6w/bv8cA iat-mode=0",  // NOLINT
    "obfs4 45.145.95.6:27015 C5B7CD6946FF10C5B3E89691A7D3F2C122D2117C cert=TD7PbUO0/0k6xYHMPW3vJxICfkMZNdkRrb63Zhl5j9dW3iRGiCx0A7mPhe5T2EDzQ35+Zw iat-mode=0",  // NOLINT
    "obfs4 51.222.13.177:80 5EDAC3B810E12B01F6FD8050D2FD3E277B289A08 cert=2uplIpLQ0q9+0qMFrK5pkaYRDOe460LL9WHBvatgkuRr/SL31wBOEupaMMJ6koRE6Ld0ew iat-mode=0",  // NOLINT
    "obfs4 37.218.245.14:38224 D9A82D2F9C2F65A18407B1D2B764F130847F8B5D cert=bjRaMrr1BRiAW8IE9U5z27fQaYgOhX1UCmOpg2pFpoMvo6ZgQMzLsaTzzQNTlm7hNcb+Sg iat-mode=0",  // NOLINT
    "obfs4 192.95.36.142:443 CDF2E852BF539B82BD10E27E9115A31734E378C2 cert=qUVQ0srL1JI/vO6V6m/24anYXiJD3QP2HgzUKQtQ7GRqqUvs7P+tG43RtAqdhLOALP7DJQ iat-mode=1",  // NOLINT
    "obfs4 85.31.186.98:443 011F2599C0E9B27EE74B353155E244813763C3E5 cert=ayq0XzCwhpdysn5o0EyDUbmSOx3X/oTEbzDMvczHOdBJKlvIdHHLJGkZARtT4dcBFArPPg iat-mode=0"  // NOLINT
  });

  static const base::NoDestructor<std::vector<std::string>> kMeekAzureBriges({
    "meek_lite 192.0.2.18:80 BE776A53492E1E044A26F17306E1BC46A55A1625 url=https://meek.azureedge.net/ front=ajax.aspnetcdn.com"  // NOLINT
  });
  // clang-format on

  switch (type) {
    case tor::BridgesConfig::BuiltinType::kSnowflake:
      return *kSnowflakeBridges;
    case tor::BridgesConfig::BuiltinType::kObfs4:
      return *kObfs4Briges;
    case tor::BridgesConfig::BuiltinType::kMeekAzure:
      return *kMeekAzureBriges;
  }
}

std::vector<std::string> LoadBridgesList(const base::Value::List* v) {
  std::vector<std::string> result;
  if (!v) {
    return result;
  }

  for (const auto& s : *v) {
    if (!s.is_string()) {
      continue;
    }
    result.push_back(s.GetString());
  }
  return result;
}

}  // namespace

namespace tor {

BridgesConfig::BridgesConfig() = default;
BridgesConfig::BridgesConfig(BridgesConfig&&) noexcept = default;
BridgesConfig::~BridgesConfig() = default;

BridgesConfig& BridgesConfig::operator=(BridgesConfig&&) noexcept = default;

const std::vector<std::string>& BridgesConfig::GetBuiltinBridges() const {
  if (auto fnd = builtin_bridges.find(use_builtin);
      fnd != builtin_bridges.end() && !fnd->second.empty()) {
    return fnd->second;
  }
  return ::GetBuiltinBridges(use_builtin);
}

void BridgesConfig::UpdateBuiltinBridges(const base::Value::Dict& dict) {
  auto load_builtin = [&](BuiltinType type) {
    auto list = LoadBridgesList(dict.FindList(GetBuiltinTypeName(type)));
    if (!list.empty()) {
      builtin_bridges[type] = std::move(list);
    }
  };
  load_builtin(BuiltinType::kSnowflake);
  load_builtin(BuiltinType::kObfs4);
  load_builtin(BuiltinType::kMeekAzure);
}

// static
std::optional<BridgesConfig> BridgesConfig::FromDict(
    const base::Value::Dict& dict) {
  BridgesConfig result;
  result.use_bridges =
      CastToEnum<Usage>(dict.FindInt(kUseBridgesKey).value_or(0));

  result.use_builtin =
      CastToEnum<BuiltinType>(dict.FindInt(kUseBuiltinBridgesKey).value_or(1));

  if (auto* bridges = dict.FindDict(kBuiltinBridgesKey)) {
    result.UpdateBuiltinBridges(*bridges);
  }
  result.provided_bridges = LoadBridgesList(dict.FindList(kProvidedBridgesKey));
  result.requested_bridges =
      LoadBridgesList(dict.FindList(kRequestedBrigesKey));
  return result;
}

// static
std::optional<BridgesConfig> BridgesConfig::FromValue(const base::Value* v) {
  if (!v || !v->is_dict()) {
    return std::nullopt;
  }
  return FromDict(v->GetDict());
}

base::Value::Dict BridgesConfig::ToDict() const {
  base::Value::Dict result;
  result.Set(kUseBridgesKey, static_cast<int>(use_bridges));
  result.Set(kUseBuiltinBridgesKey, static_cast<int>(use_builtin));

  auto save_list = [](const std::vector<std::string>& bridges) {
    base::Value::List list;
    for (const auto& bridge : bridges) {
      list.Append(bridge);
    }
    return list;
  };

  base::Value::Dict builtin;
  for (const auto& v : builtin_bridges) {
    builtin.Set(GetBuiltinTypeName(v.first), save_list(v.second));
  }

  result.Set(kBuiltinBridgesKey, std::move(builtin));
  result.Set(kProvidedBridgesKey, save_list(provided_bridges));
  result.Set(kRequestedBrigesKey, save_list(requested_bridges));

  return result;
}

base::Value BridgesConfig::ToValue() const {
  return base::Value(ToDict());
}

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state) {
  // Do this for legacy tor profile migration because tor profile might be
  // last active profile before upgrading
  std::string last_used_profile_name =
      local_state->GetString(prefs::kProfileLastUsed);
  if (!last_used_profile_name.empty() &&
      last_used_profile_name ==
          base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe()) {
    local_state->SetString(prefs::kProfileLastUsed, chrome::kInitialProfile);
  }
}

}  // namespace tor

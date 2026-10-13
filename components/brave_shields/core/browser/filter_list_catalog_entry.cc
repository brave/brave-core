// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/brave_shields_locale_utils.h"
#include "crypto/sha2.h"

namespace {

// `value` is untrusted remote data (the downloaded filter-list catalog), so
// a malformed or non-base64 key is rejected here rather than crashing
// downstream when it's eventually decoded.
bool GetPublicKeySHA256(
    const base::Value* value,
    std::optional<std::array<uint8_t, crypto::kSHA256Length>>* field) {
  if (value == nullptr || !value->is_dict()) {
    return false;
  }
  const base::DictValue& dict = value->GetDict();
  const auto* base64_public_key = dict.FindString("base64_public_key");
  if (!base64_public_key) {
    return false;
  }
  auto decoded_public_key = base::Base64Decode(*base64_public_key);
  if (!decoded_public_key) {
    return false;
  }
  *field = crypto::SHA256Hash(*decoded_public_key);
  return true;
}

bool GetStringVector(const base::Value* value,
                     std::vector<std::string>* field) {
  DCHECK(field);
  field->clear();
  if (value == nullptr || !value->is_list()) {
    return false;
  } else {
    const base::ListValue& list = value->GetList();
    for (const auto& list_value : list) {
      const auto* s = list_value.GetIfString();
      if (s) {
        field->push_back(*s);
      } else {
        return false;
      }
    }
    return true;
  }
}

bool GetUint8(const base::Value* value, uint8_t* field) {
  DCHECK(field);
  if (value == nullptr || !value->is_int()) {
    return false;
  } else {
    int i = value->GetInt();
    if (!base::IsValueInRangeForNumericType<uint8_t>(i)) {
      return false;
    }
    *field = base::checked_cast<uint8_t>(i);
    return true;
  }
}

#if BUILDFLAG(IS_LINUX)
constexpr char kCurrentPlatform[] = "LINUX";
#elif BUILDFLAG(IS_WIN)
constexpr char kCurrentPlatform[] = "WINDOWS";
#elif BUILDFLAG(IS_MAC)
constexpr char kCurrentPlatform[] = "MAC";
#elif BUILDFLAG(IS_ANDROID)
constexpr char kCurrentPlatform[] = "ANDROID";
#elif BUILDFLAG(IS_IOS)
constexpr char kCurrentPlatform[] = "IOS";
#else
constexpr char kCurrentPlatform[] = "OTHER";
#endif

}  // namespace

namespace brave_shields {

FilterListCatalogEntry::FilterListCatalogEntry() {}

FilterListCatalogEntry::FilterListCatalogEntry(
    const std::string& uuid,
    const std::string& url,
    const std::string& title,
    const std::vector<std::string>& langs,
    const std::string& support_url,
    const std::string& desc,
    bool hidden,
    bool default_enabled,
    bool first_party_protections,
    uint8_t permission_mask,
    const std::vector<std::string>& platforms,
    base::span<const uint8_t, crypto::kSHA256Length> public_key_sha256)
    : uuid(uuid),
      url(url),
      title(title),
      langs(langs),
      support_url(support_url),
      desc(desc),
      hidden(hidden),
      default_enabled(default_enabled),
      first_party_protections(first_party_protections),
      permission_mask(permission_mask),
      platforms(platforms) {
  this->public_key_sha256 =
      std::make_optional<std::array<uint8_t, crypto::kSHA256Length>>();
  base::span(this->public_key_sha256.value()).copy_from(public_key_sha256);
}

FilterListCatalogEntry::FilterListCatalogEntry(
    const FilterListCatalogEntry& other) = default;

FilterListCatalogEntry::~FilterListCatalogEntry() = default;

void FilterListCatalogEntry::RegisterJSONConverter(
    base::JSONValueConverter<FilterListCatalogEntry>* converter) {
  converter->RegisterStringField("uuid", &FilterListCatalogEntry::uuid);
  converter->RegisterStringField("url", &FilterListCatalogEntry::url);
  converter->RegisterStringField("title", &FilterListCatalogEntry::title);
  converter->RegisterCustomValueField<std::vector<std::string>>(
      "langs", &FilterListCatalogEntry::langs, &GetStringVector);
  converter->RegisterStringField("support_url",
                                 &FilterListCatalogEntry::support_url);
  converter->RegisterStringField("desc", &FilterListCatalogEntry::desc);
  converter->RegisterBoolField("hidden", &FilterListCatalogEntry::hidden);
  converter->RegisterBoolField("default_enabled",
                               &FilterListCatalogEntry::default_enabled);
  converter->RegisterBoolField(
      "first_party_protections",
      &FilterListCatalogEntry::first_party_protections);
  converter->RegisterCustomValueField(
      "permission_mask", &FilterListCatalogEntry::permission_mask, &GetUint8);
  converter->RegisterCustomValueField(
      "list_text_component", &FilterListCatalogEntry::public_key_sha256,
      &GetPublicKeySHA256);
  converter->RegisterCustomValueField<std::vector<std::string>>(
      "platforms", &FilterListCatalogEntry::platforms, &GetStringVector);
}

bool FilterListCatalogEntry::SupportsCurrentPlatform() const {
  if (platforms.empty()) {
    return true;
  }

  return std::find(platforms.begin(), platforms.end(), kCurrentPlatform) !=
         platforms.end();
}

std::vector<FilterListCatalogEntry>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& uuid) {
  return std::ranges::find(region_lists, uuid, &FilterListCatalogEntry::uuid);
}

// Given a locale like `en-US`, find regional lists corresponding to the
// language (`en`) part.
std::vector<std::reference_wrapper<FilterListCatalogEntry const>>
FindAdBlockFilterListsByLocale(
    const std::vector<FilterListCatalogEntry>& region_lists,
    const std::string& locale) {
  std::vector<std::reference_wrapper<FilterListCatalogEntry const>> output;

  const std::string adjusted_locale = GetLanguageCodeFromLocale(locale);
  std::copy_if(region_lists.begin(), region_lists.end(),
               std::back_inserter(output),
               [&adjusted_locale](const FilterListCatalogEntry& entry) {
                 return std::ranges::contains(entry.langs, adjusted_locale);
               });

  return output;
}

std::vector<FilterListCatalogEntry> FilterListCatalogFromJSON(
    const std::string& catalog_json) {
  std::vector<FilterListCatalogEntry> catalog =
      std::vector<FilterListCatalogEntry>();

  std::optional<base::ListValue> parsed_json = base::JSONReader::ReadList(
      catalog_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed_json) {
    LOG(ERROR) << "Could not load regional adblock catalog";
    return catalog;
  }

  base::JSONValueConverter<FilterListCatalogEntry> converter;

  for (const auto& item : *parsed_json) {
    FilterListCatalogEntry entry;
    if (!converter.Convert(item, &entry) || !entry.public_key_sha256) {
      continue;
    }
    catalog.push_back(entry);
  }

  return catalog;
}

}  // namespace brave_shields

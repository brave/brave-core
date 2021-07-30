/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/conversions_resource.h"

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/result.h"

namespace ads {
namespace resource {

namespace {
const char kResourceId[] = "nnqccijfhvzwyrxpxwjrpmynaiazctqb";
const int kVersionId = 1;
}  // namespace

Conversions::Conversions() = default;

Conversions::~Conversions() = default;

bool Conversions::IsInitialized() const {
  return is_initialized_;
}

void Conversions::Load() {
  AdsClientHelper::Get()->LoadAdsResource(
      kResourceId, kVersionId,
      [=](const Result result, const std::string& json) {
        if (result != SUCCESS) {
          BLOG(1, "Failed to load resource " << kResourceId);
          is_initialized_ = false;
          return;
        }

        BLOG(1, "Successfully loaded resource " << kResourceId);

        if (!FromJson(json)) {
          BLOG(1, "Failed to initialize resource " << kResourceId);
          is_initialized_ = false;
          return;
        }

        is_initialized_ = true;

        BLOG(1, "Successfully initialized resource " << kResourceId);
      });
}

ConversionIdPatternMap Conversions::get() const {
  return conversion_id_patterns_;
}

///////////////////////////////////////////////////////////////////////////////

bool Conversions::FromJson(const std::string& json) {
  ConversionIdPatternMap conversion_id_patterns;

  absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root) {
    BLOG(1, "Failed to load from JSON, root missing");
    return false;
  }

  if (absl::optional<int> version = root->FindIntPath("version")) {
    if (kVersionId != *version) {
      BLOG(1, "Failed to load from JSON, version missing");
      return false;
    }
  }

  base::Value* conversion_id_patterns_value =
      root->FindDictPath("conversion_id_patterns");
  if (!conversion_id_patterns_value) {
    BLOG(1, "Failed to load from JSON, conversion patterns missing");
    return false;
  }

  if (!conversion_id_patterns_value->is_dict()) {
    BLOG(1, "Failed to load from JSON, conversion patterns not of type dict");
    return false;
  }

  base::DictionaryValue* dict;
  if (!conversion_id_patterns_value->GetAsDictionary(&dict)) {
    BLOG(1, "Failed to load from JSON, get conversion patterns as dict");
    return false;
  }

  for (base::DictionaryValue::Iterator iter(*dict); !iter.IsAtEnd();
       iter.Advance()) {
    if (!iter.value().is_dict()) {
      BLOG(1, "Failed to load from JSON, conversion pattern not of type dict")
      return false;
    }

    const std::string* id_pattern = iter.value().FindStringKey("id_pattern");
    if (id_pattern->empty()) {
      BLOG(1, "Failed to load from JSON, pattern id_pattern missing");
      return false;
    }

    const std::string* search_in = iter.value().FindStringKey("search_in");
    if (search_in->empty()) {
      BLOG(1, "Failed to load from JSON, pattern search_in missing");
      return false;
    }

    ConversionIdPatternInfo info;
    info.id_pattern = *id_pattern;
    info.search_in = *search_in;
    info.url_pattern = iter.key();
    conversion_id_patterns.insert({info.url_pattern, info});
  }

  conversion_id_patterns_ = conversion_id_patterns;

  BLOG(1, "Parsed verifiable conversion resource version " << kVersionId);

  return true;
}

}  // namespace resource
}  // namespace ads

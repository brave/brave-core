/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/features/anti_targeting/anti_targeting_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/result.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace resource {

namespace {
const char kResourceId[] = "mkdhnfmjhklfnamlheoliekgeohamoig";
}  // namespace

AntiTargeting::AntiTargeting() = default;

AntiTargeting::~AntiTargeting() = default;

bool AntiTargeting::IsInitialized() const {
  return is_initialized_;
}

void AntiTargeting::Load() {
  AdsClientHelper::Get()->LoadAdsResource(
      kResourceId, features::GetAntiTargetingResourceVersion(),
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

AntiTargetingInfo AntiTargeting::get() const {
  return anti_targeting_;
}

///////////////////////////////////////////////////////////////////////////////

bool AntiTargeting::FromJson(const std::string& json) {
  AntiTargetingInfo anti_targeting;

  base::Optional<base::Value> root = base::JSONReader::Read(json);
  if (!root) {
    BLOG(1, "Failed to load from JSON, root missing");
    return false;
  }

  if (base::Optional<int> version = root->FindIntPath("version")) {
    if (features::GetAntiTargetingResourceVersion() != *version) {
      BLOG(1, "Failed to load from JSON, version missing");
      return false;
    }

    anti_targeting.version = *version;
  }

  base::Value* site_lists_value = root->FindDictPath("sites");
  if (!site_lists_value) {
    BLOG(1, "Failed to load from JSON, sites missing");
    return false;
  }

  if (!site_lists_value->is_dict()) {
    BLOG(1, "Failed to load from JSON, sites not of type dict");
    return false;
  }

  base::DictionaryValue* dict;
  if (!site_lists_value->GetAsDictionary(&dict)) {
    BLOG(1, "Failed to load from JSON, get sites as dict");
    return false;
  }

  for (base::DictionaryValue::Iterator iter(*dict); !iter.IsAtEnd();
       iter.Advance()) {
    if (!iter.value().is_list()) {
      BLOG(1, "Failed to load from JSON, sites not of type list")
      return false;
    }

    std::vector<std::string> sites;
    for (const auto& site : iter.value().GetList()) {
      sites.push_back(site.GetString());
    }

    anti_targeting.sites.insert({iter.key(), sites});
  }

  anti_targeting_ = anti_targeting;

  BLOG(1, "Parsed anti targeting resource version " << anti_targeting_.version);

  return true;
}

}  // namespace resource
}  // namespace ads

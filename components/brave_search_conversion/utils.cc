/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search_conversion/utils.h"

#include <algorithm>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_search_conversion/constants.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "url/gurl.h"

namespace brave_search_conversion {

namespace {

bool ShouldUseDuckDuckGoBanner(const TemplateURL* template_url) {
  if (!base::FeatureList::IsEnabled(features::kOmniboxDDGBanner)) {
    return false;
  }

  const auto id = template_url->prepopulate_id();
  if (id == TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO ||
      id == TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE ||
      id == TemplateURLPrepopulateData::
                PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE) {
    return true;
  }

  // If user adds manualy as default search provider, it could not have above
  // id. So, check with host again.
  if (GURL(template_url->url()).host() == "duckduckgo.com") {
    return true;
  }

  return false;
}

ConversionType GetConversionTypeFromBannerTypeParam(const std::string& param) {
  if (param == "type_B") {
    return ConversionType::kBannerTypeB;
  }

  if (param == "type_C") {
    return ConversionType::kBannerTypeC;
  }

  if (param == "type_D") {
    return ConversionType::kBannerTypeD;
  }

  if (param == "type_DDG_A") {
    return ConversionType::kDDGBannerTypeA;
  }

  if (param == "type_DDG_B") {
    return ConversionType::kDDGBannerTypeB;
  }

  if (param == "type_DDG_C") {
    return ConversionType::kDDGBannerTypeC;
  }

  if (param == "type_DDG_D") {
    return ConversionType::kDDGBannerTypeD;
  }

  LOG(ERROR) << __func__
             << " : Got invalid conversion type from griffin: " << param;
  return ConversionType::kNone;
}

}  // namespace

bool IsNTPPromotionEnabled(PrefService* prefs, TemplateURLService* service) {
  DCHECK(prefs);
  DCHECK(service);

  if (prefs->GetBoolean(prefs::kDismissed)) {
    return false;
  }

  // Don't need to prompt for conversion if user uses brave as a default
  // provider.
  auto* template_url = service->GetDefaultSearchProvider();
  if (!template_url) {
    return false;
  }

  const auto id = template_url->prepopulate_id();
  if (id == TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE ||
      id == TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE_TOR) {
    return false;
  }

  return base::FeatureList::IsEnabled(features::kNTP);
}

ConversionType GetConversionType(PrefService* prefs,
                                 TemplateURLService* service) {
  DCHECK(prefs);
  DCHECK(service);

  if (prefs->GetBoolean(prefs::kDismissed)) {
    return ConversionType::kNone;
  }

  // Don't need to ask conversion if user uses brave as a default provider.
  const auto* template_url = service->GetDefaultSearchProvider();
  if (template_url->prepopulate_id() ==
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE ||
      template_url->prepopulate_id() ==
          TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE_TOR) {
    return ConversionType::kNone;
  }

  if (IsBraveSearchConversionFeatureEnabled()) {
    // Give conversion type after 3d passed since maybe later clicked time.
    auto clicked_time = prefs->GetTime(prefs::kMaybeLaterClickedTime);
    if (!clicked_time.is_null() &&
        clicked_time + base::Days(3) > base::Time::Now()) {
      return ConversionType::kNone;
    }

    if (ShouldUseDuckDuckGoBanner(template_url)) {
      return GetConversionTypeFromBannerTypeParam(
          features::kDDGBannerType.Get());
    }

    if (base::FeatureList::IsEnabled(features::kOmniboxBanner)) {
      return GetConversionTypeFromBannerTypeParam(features::kBannerType.Get());
    }

    return ConversionType::kNone;
  }

  return ConversionType::kNone;
}

void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kDismissed, false);
  registry->RegisterBooleanPref(prefs::kShowNTPSearchBox, true);
  registry->RegisterBooleanPref(prefs::kPromptEnableSuggestions, true);
  registry->RegisterTimePref(prefs::kMaybeLaterClickedTime, base::Time());
}

void SetDismissed(PrefService* prefs) {
  prefs->SetBoolean(prefs::kDismissed, true);
}

void SetMaybeLater(PrefService* prefs) {
  prefs->SetTime(prefs::kMaybeLaterClickedTime, base::Time::Now());
}

GURL GetPromoURL(const std::u16string& search_term) {
  return GetPromoURL(base::UTF16ToUTF8(search_term));
}

GURL GetPromoURL(const std::string& search_term) {
  std::string promo_url(kBraveSearchConversionPromotionURL);
  base::ReplaceSubstringsAfterOffset(&promo_url, 0, kSearchTermsParameter,
                                     search_term);
  return GURL(promo_url);
}

bool IsBraveSearchConversionFeatureEnabled() {
  return base::FeatureList::IsEnabled(features::kOmniboxBanner) ||
         base::FeatureList::IsEnabled(features::kOmniboxDDGBanner);
}

}  // namespace brave_search_conversion

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search_conversion/utils.h"

#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_search_conversion/constants.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_service.h"
#include "url/gurl.h"

namespace brave_search_conversion {

bool IsPromotionEnabledCountry(const std::string& country_code) {
  static const base::flat_set<std::string> supported_country{"US", "CA", "DE",
                                                             "FR", "GB"};
  return supported_country.find(country_code) != supported_country.end();
}

ConversionType GetConversionType(PrefService* prefs,
                                 TemplateURLService* service) {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  if (!IsPromotionEnabledCountry(country_code))
    return ConversionType::kNone;

  // Skip |prefs| or |service| if they are null.
  // If both are null, only check each feature's state.
  if (prefs && prefs->GetBoolean(prefs::kDismissed))
    return ConversionType::kNone;

  if (service) {
    // Don't need to ask conversion if user uses brave as a default provider.
    auto id = service->GetDefaultSearchProvider()->data().prepopulate_id;
    if (id == TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE ||
        id == TemplateURLPrepopulateData::PREPOPULATED_ENGINE_ID_BRAVE_TOR) {
      return ConversionType::kNone;
    }
  }

  if (base::FeatureList::IsEnabled(features::kOmniboxButton))
    return ConversionType::kButton;

  if (base::FeatureList::IsEnabled(features::kOmniboxBanner))
    return ConversionType::kBanner;

  return ConversionType::kNone;
}

void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kDismissed, false);
}

void SetDismissed(PrefService* prefs) {
  prefs->SetBoolean(prefs::kDismissed, true);
}

GURL GetPromoURL(const std::u16string& search_term) {
  std::string promo_url(kBraveSearchConversionPromotionURL);
  base::ReplaceSubstringsAfterOffset(&promo_url, 0, kSearchTermsParameter,
                                     base::UTF16ToUTF8(search_term));
  return GURL(promo_url);
}

}  // namespace brave_search_conversion

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_UTILS_H_

#include <string>

class GURL;
class PrefRegistrySimple;
class PrefService;
class TemplateURLService;

namespace brave_search_conversion {

enum class ConversionType;

ConversionType GetConversionType(PrefService* prefs,
                                 TemplateURLService* service);
void RegisterPrefs(PrefRegistrySimple* registry);
void SetDismissed(PrefService* prefs);
GURL GetPromoURL(const std::u16string& search_term);
bool IsPromotionEnabledCountry(const std::string& country_code);
bool IsBraveSearchConversionFetureEnabled();

}  // namespace brave_search_conversion

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_UTILS_H_

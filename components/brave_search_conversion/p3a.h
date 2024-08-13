// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_P3A_H_

#include "brave/components/brave_search_conversion/types.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_search_conversion {
namespace p3a {

inline constexpr char kSearchPromoButtonHistogramName[] =
    "Brave.Search.Promo.Button";
inline constexpr char kSearchPromoBannerBHistogramName[] =
    "Brave.Search.Promo.BannerB";
inline constexpr char kSearchPromoBannerCHistogramName[] =
    "Brave.Search.Promo.BannerC";
inline constexpr char kSearchPromoBannerDHistogramName[] =
    "Brave.Search.Promo.BannerD";
inline constexpr char kSearchPromoDDGBannerCHistogramName[] =
    "Brave.Search.Promo.DDGBannerC";
inline constexpr char kSearchPromoDDGBannerDHistogramName[] =
    "Brave.Search.Promo.DDGBannerD";

inline constexpr char kSearchPromoNTPHistogramName[] =
    "Brave.Search.Promo.NewTabPage";
inline constexpr char kSearchQueriesBeforeChurnHistogramName[] =
    "Brave.Search.QueriesBeforeChurn";

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteLocalStatePrefs(PrefService* local_state);

void RecordPromoShown(PrefService* prefs, ConversionType type);
void RecordPromoTrigger(PrefService* prefs, ConversionType type);

void RecordLocationBarQuery(PrefService* prefs);
void RecordDefaultEngineConversion(PrefService* prefs);
void RecordDefaultEngineChurn(PrefService* prefs);

}  // namespace p3a
}  // namespace brave_search_conversion

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_P3A_H_

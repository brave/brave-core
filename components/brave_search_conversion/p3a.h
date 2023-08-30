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

extern const char kSearchPromoButtonHistogramName[];
extern const char kSearchPromoBannerAHistogramName[];
extern const char kSearchPromoBannerBHistogramName[];
extern const char kSearchPromoBannerCHistogramName[];
extern const char kSearchPromoBannerDHistogramName[];
extern const char kSearchPromoNTPHistogramName[];

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteLocalStatePrefs(PrefService* local_state);
void RecordPromoShown(PrefService* prefs, ConversionType type);
void RecordPromoTrigger(PrefService* prefs, ConversionType type);
void RecordDefaultEngineChange(PrefService* prefs);

}  // namespace p3a
}  // namespace brave_search_conversion

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_P3A_H_

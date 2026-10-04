/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_CONSTANTS_H_

namespace brave_search_conversion {

inline constexpr char kBraveSearchConversionPromotionURL[] =
    "https://search.brave.com/search?q={SearchTerms}&action=makeDefault";
inline constexpr char kSearchTermsParameter[] = "{SearchTerms}";

}  // namespace brave_search_conversion

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_CONVERSION_CONSTANTS_H_

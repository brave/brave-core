/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// KeywordList type alias separated from the keyphrase parser so that struct
// headers needing only the type do not pull in ParseKeyphrase().

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_KEYPHRASE_PURCHASE_INTENT_KEYPHRASE_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_KEYPHRASE_PURCHASE_INTENT_KEYPHRASE_TYPES_H_

#include <string>
#include <vector>

namespace brave_ads {

using KeywordList = std::vector<std::string>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_KEYPHRASE_PURCHASE_INTENT_KEYPHRASE_TYPES_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/browsing_data/core/browsing_data_utils.h"

#define GetCounterTextFromResult GetCounterTextFromResult_ChromiumImpl  // NOLINT
#define GetDataTypeFromDeletionPreference GetDataTypeFromDeletionPreference_ChromiumImpl  // NOLINT

// Can't override method because of below error.
// error: enumeration value 'SHIELDS_SETTINGS' not handled in switch.
#define HANDLE_BROWSING_DATA_TYPE_SHIELDS_SETTINGS \
  case BrowsingDataType::SHIELDS_SETTINGS: \
    *out_pref = prefs::kDeleteShieldsSettings; \
    return true;

#include "../../../../../components/browsing_data/core/browsing_data_utils.cc"  // NOLINT

#undef GetCounterTextFromResult
#undef GetDataTypeFromDeletionPreference
#undef HANDLE_BROWSING_DATA_TYPE_SHIELDS_SETTINGS

namespace browsing_data {

base::string16 GetCounterTextFromResult(
    const BrowsingDataCounter::Result* result) {
  std::string pref_name = result->source()->GetPrefName();
  if (pref_name == prefs::kDeleteShieldsSettings) {
    BrowsingDataCounter::ResultInt count =
        static_cast<const BrowsingDataCounter::FinishedResult*>(result)
            ->Value();
    return
        l10n_util::GetPluralStringFUTF16(IDS_DEL_SITE_SETTINGS_COUNTER, count);
  }
  return GetCounterTextFromResult_ChromiumImpl(result);
}

BrowsingDataType GetDataTypeFromDeletionPreference(
    const std::string& pref_name) {
  if (pref_name == prefs::kDeleteShieldsSettings)
    return BrowsingDataType::SHIELDS_SETTINGS;
  return GetDataTypeFromDeletionPreference_ChromiumImpl(pref_name);
}

}  // namespace browsing_data

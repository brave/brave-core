/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_COUNTRY_SUPPORTED_COUNTRY_CODES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_COUNTRY_SUPPORTED_COUNTRY_CODES_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_piece.h"

namespace ads::geographic {

using SupportedCountryCodeSet = base::flat_set<base::StringPiece>;
using SupportedCountryCodeMap = base::flat_map<int, SupportedCountryCodeSet>;

const SupportedCountryCodeMap& GetSupportedCountryCodes();

}  // namespace ads::geographic

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_COUNTRY_SUPPORTED_COUNTRY_CODES_H_

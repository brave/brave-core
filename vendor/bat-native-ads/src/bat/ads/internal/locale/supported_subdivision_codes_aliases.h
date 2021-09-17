/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_SUBDIVISION_CODES_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_SUBDIVISION_CODES_ALIASES_H_

#include <map>
#include <set>
#include <string>

namespace ads {

using SupportedSubdivisionCodesSet = std::set<std::string>;
using SupportedSubdivisionCodesMap =
    std::map<std::string, SupportedSubdivisionCodesSet>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_SUBDIVISION_CODES_ALIASES_H_

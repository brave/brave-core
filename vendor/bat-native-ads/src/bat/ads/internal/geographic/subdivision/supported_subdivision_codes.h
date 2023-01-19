/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUPPORTED_SUBDIVISION_CODES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUPPORTED_SUBDIVISION_CODES_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/strings/string_piece_forward.h"
#include "bat/ads/supported_subdivisions.h"

namespace ads::geographic {

using SupportedSubdivisionCodesMap =
    base::flat_map<base::StringPiece, SupportedSubdivisions>;

const SupportedSubdivisionCodesMap& GetSupportedSubdivisionCodes();

bool IsSupportedSubdivisionCode(const std::string& country_code,
                                const std::string& subdivision_code);

}  // namespace ads::geographic

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_GEOGRAPHIC_SUBDIVISION_SUPPORTED_SUBDIVISION_CODES_H_

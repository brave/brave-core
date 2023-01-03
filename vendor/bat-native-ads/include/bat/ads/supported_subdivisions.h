/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_SUPPORTED_SUBDIVISIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_SUPPORTED_SUBDIVISIONS_H_

#include "base/containers/flat_map.h"
#include "base/strings/string_piece_forward.h"
#include "bat/ads/export.h"

namespace ads {

using SupportedSubdivisions =
    base::flat_map<base::StringPiece, base::StringPiece>;

ADS_EXPORT SupportedSubdivisions GetSupportedSubdivisions();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_SUPPORTED_SUBDIVISIONS_H_

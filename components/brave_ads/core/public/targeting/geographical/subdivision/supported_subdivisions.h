/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_

#include "base/containers/flat_map.h"
#include "base/strings/string_piece_forward.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/public/export.h"

namespace brave_ads {

using SupportedSubdivisions = base::flat_map</*subdivision*/ base::StringPiece,
                                             /*name*/ base::StringPiece>;

using SupportedSubdivisionMap =
    base::flat_map</*country_code*/ base::StringPiece, SupportedSubdivisions>;

ADS_EXPORT const SupportedSubdivisionMap& GetSupportedSubdivisions();

ADS_EXPORT base::Value::List GetSupportedSubdivisionsAsValueList();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_

#include <string_view>

#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/public/export.h"

namespace brave_ads {

using SubdivisionMap = base::flat_map</*subdivision*/ std::string_view,
                                      /*name*/ std::string_view>;

using SupportedSubdivisionMap =
    base::flat_map</*country_code*/ std::string_view, SubdivisionMap>;

ADS_EXPORT const SupportedSubdivisionMap& GetSupportedSubdivisions();

base::Value::List GetSupportedSubdivisionsAsValueList(
    const std::string& country_code);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_

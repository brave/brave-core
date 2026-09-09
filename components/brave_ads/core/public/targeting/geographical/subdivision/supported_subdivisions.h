/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_

#include <string_view>

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

using SubdivisionMap = absl::flat_hash_map</*subdivision*/ std::string_view,
                                           /*name*/ std::string_view>;

using SupportedSubdivisionMap =
    absl::flat_hash_map</*country_code*/ std::string_view, SubdivisionMap>;

const SupportedSubdivisionMap& GetSupportedSubdivisions();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_TARGETING_GEOGRAPHICAL_SUBDIVISION_SUPPORTED_SUBDIVISIONS_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CONSTANTS_H_

#include "brave/components/brave_ads/core/public/export.h"

namespace brave_ads {

inline constexpr char kDatabaseFilename[] = "database.sqlite";

// TODO(https://github.com/brave/brave-browser/issues/39795): Transition away
// from using JSON state to a more efficient data approach.
inline constexpr char kClientJsonFilename[] = "client.json";

// TODO(https://github.com/brave/brave-browser/issues/39795): Transition away
// from using JSON state to a more efficient data approach.
inline constexpr char kConfirmationsJsonFilename[] = "confirmations.json";

inline constexpr char kCatalogJsonSchemaDataResourceName[] =
    "catalog-schema.json";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CONSTANTS_H_

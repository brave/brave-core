/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_SUPPORTED_SUBDIVISIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_SUPPORTED_SUBDIVISIONS_H_

#include "base/containers/flat_map.h"
#include "base/strings/string_piece_forward.h"
#include "brave/components/brave_ads/core/export.h"

namespace ads {

using SupportedSubdivisions =
    base::flat_map<base::StringPiece, base::StringPiece>;

ADS_EXPORT SupportedSubdivisions GetSupportedSubdivisions();

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_SUPPORTED_SUBDIVISIONS_H_

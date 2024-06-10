/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_PARSING_ERROR_OR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_PARSING_ERROR_OR_H_

#include <string>

#include "base/types/expected.h"

namespace brave_ads {

// Helper for methods which perform file read operations to parse a json file,
// and initiate a given type.
template <class ValueType>
using ResourceComponentParsingErrorOr = base::expected<ValueType, std::string>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_PARSING_ERROR_OR_H_

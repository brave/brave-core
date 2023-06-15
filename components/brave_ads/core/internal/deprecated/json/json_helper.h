/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_JSON_JSON_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_JSON_JSON_HELPER_H_

#ifdef _MSC_VER
// Resolve Windows build issue due to Windows globally defining GetObject which
// causes RapidJson to fail
#undef GetObject
#endif

#include <string>

#include "brave/third_party/rapidjson/src/include/rapidjson/document.h"

namespace brave_ads::helper::json {

[[nodiscard]] bool Validate(rapidjson::Document* document,
                            const std::string& json_schema);

std::string GetLastError(rapidjson::Document* document);

}  // namespace brave_ads::helper::json

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_JSON_JSON_HELPER_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_UTILS_CUSTOM_ATTRIBUTES_H_
#define BRAVE_COMPONENTS_P3A_UTILS_CUSTOM_ATTRIBUTES_H_

#include <optional>
#include <string_view>

namespace p3a_utils {

// Sets a custom attribute value, which can be included
// in any metric report, depending on metric configuration.
// If nullopt is provided for attribute_value, the attribute
// will be cleared.
void SetCustomAttribute(std::string_view attribute_name,
                        std::optional<std::string_view> attribute_value);

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_CUSTOM_ATTRIBUTES_H_

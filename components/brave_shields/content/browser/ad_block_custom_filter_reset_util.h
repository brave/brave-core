// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_CUSTOM_FILTER_RESET_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_CUSTOM_FILTER_RESET_UTIL_H_

#include <optional>
#include <string>

namespace brave_shields {

std::optional<std::string> ResetCustomFiltersForHost(
    std::string_view host,
    std::string_view custom_filters);

bool IsCustomFiltersAvailable(std::string_view host,
                              std::string_view custom_filters);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_CUSTOM_FILTER_RESET_UTIL_H_

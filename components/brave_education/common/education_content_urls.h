/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_EDUCATION_COMMON_EDUCATION_CONTENT_URLS_H_
#define BRAVE_COMPONENTS_BRAVE_EDUCATION_COMMON_EDUCATION_CONTENT_URLS_H_

#include <optional>
#include <string_view>

#include "url/gurl.h"

namespace brave_education {

enum class EducationContentType { kGettingStarted };

// Returns a WebUI URL for displaying the specified education content type.
GURL GetEducationContentBrowserURL(EducationContentType content_type);

// Returns a website URL that will be loaded into an iframe for the specified
// education content type.
GURL GetEducationContentServerURL(EducationContentType content_type);

// Returns the education content type that corresponds to the specified WebUI
// URL. A WebUI can use this function to determine which content type to show
// for the current URL.
std::optional<EducationContentType> EducationContentTypeFromBrowserURL(
    std::string_view browser_url);

}  // namespace brave_education

#endif  // BRAVE_COMPONENTS_BRAVE_EDUCATION_COMMON_EDUCATION_CONTENT_URLS_H_

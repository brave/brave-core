/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_EDUCATION_EDUCATION_URLS_H_
#define BRAVE_COMPONENTS_BRAVE_EDUCATION_EDUCATION_URLS_H_

#include <optional>

#include "url/gurl.h"

namespace brave_education {

enum class EducationPageType { kGettingStarted };

// Returns a WebUI URL for displaying the specified education page type.
GURL GetEducationPageBrowserURL(EducationPageType page_type);

// Returns a website URL that will be loaded into an iframe for the specified
// education page type.
GURL GetEducationPageServerURL(EducationPageType page_type);

// Returns the education page type that corresponds to the specified WebUI URL.
// A WebUI can use this function to determine which page type to show for the
// current URL.
std::optional<EducationPageType> EducationPageTypeFromBrowserURL(
    GURL browser_url);

}  // namespace brave_education

#endif  // BRAVE_COMPONENTS_BRAVE_EDUCATION_EDUCATION_URLS_H_

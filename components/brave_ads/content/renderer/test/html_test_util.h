/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_HTML_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_HTML_TEST_UTIL_H_

#include <cstddef>
#include <string>

namespace brave_ads::test {

// Wraps `inner_html` in `nesting_depth` levels of `<div>`, for exercising
// DOM-depth-bound behavior.
std::string WrapInNestedDivs(size_t nesting_depth,
                             const std::string& inner_html);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_RENDERER_TEST_HTML_TEST_UTIL_H_

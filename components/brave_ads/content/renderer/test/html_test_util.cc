/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/renderer/test/html_test_util.h"

namespace brave_ads::test {

std::string WrapInNestedDivs(size_t nesting_depth,
                             const std::string& inner_html) {
  std::string html;
  for (size_t i = 0; i < nesting_depth; ++i) {
    html += "<div>";
  }
  html += inner_html;
  for (size_t i = 0; i < nesting_depth; ++i) {
    html += "</div>";
  }
  return html;
}

}  // namespace brave_ads::test

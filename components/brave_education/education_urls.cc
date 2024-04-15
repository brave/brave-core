/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_education/education_urls.h"

#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "brave/components/constants/webui_url_constants.h"

namespace brave_education {

namespace {

constexpr std::string_view PageTypeToBrowserURL(EducationPageType page_type) {
  switch (page_type) {
    case EducationPageType::kGettingStarted:
      return kBraveGettingStartedURL;
  }
}

constexpr std::string_view PageTypeToServerURL(EducationPageType page_type) {
  switch (page_type) {
    case EducationPageType::kGettingStarted:
      return "https://brave.com/getting-started/";
  }
}

consteval auto MapEntry(EducationPageType page_type) {
  return std::make_pair(PageTypeToBrowserURL(page_type), page_type);
}

constexpr auto kBrowserURLMap =
    base::MakeFixedFlatMap({MapEntry(EducationPageType::kGettingStarted)});

}  // namespace

GURL GetEducationPageBrowserURL(EducationPageType page_type) {
  return GURL(PageTypeToBrowserURL(page_type));
}

GURL GetEducationPageServerURL(EducationPageType page_type) {
  return GURL(PageTypeToServerURL(page_type));
}

std::optional<EducationPageType> EducationPageTypeFromBrowserURL(
    GURL browser_url) {
  auto iter = kBrowserURLMap.find(browser_url.spec());
  if (iter == kBrowserURLMap.end()) {
    return std::nullopt;
  }
  return iter->second;
}

}  // namespace brave_education

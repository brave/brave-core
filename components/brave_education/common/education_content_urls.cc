/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_education/common/education_content_urls.h"

#include <map>
#include <utility>

#include "base/containers/fixed_flat_map.h"

namespace brave_education {

namespace {

constexpr std::string_view TypeToBrowserURL(EducationContentType content_type) {
  switch (content_type) {
    case EducationContentType::kGettingStarted:
      return "chrome://getting-started/";
  }
}

constexpr std::string_view TypeToServerURL(EducationContentType content_type) {
  switch (content_type) {
    case EducationContentType::kGettingStarted:
      return "https://brave.com/get-started/";
  }
}

consteval auto MapEntry(EducationContentType content_type) {
  return std::make_pair(TypeToBrowserURL(content_type), content_type);
}

constexpr auto kBrowserURLMap =
    base::MakeFixedFlatMap({MapEntry(EducationContentType::kGettingStarted)});

}  // namespace

GURL GetEducationContentBrowserURL(EducationContentType content_type) {
  return GURL(TypeToBrowserURL(content_type));
}

GURL GetEducationContentServerURL(EducationContentType content_type) {
  return GURL(TypeToServerURL(content_type));
}

std::optional<EducationContentType> EducationContentTypeFromBrowserURL(
    std::string_view browser_url) {
  auto iter = kBrowserURLMap.find(browser_url);
  if (iter == kBrowserURLMap.end()) {
    return std::nullopt;
  }
  return iter->second;
}

}  // namespace brave_education

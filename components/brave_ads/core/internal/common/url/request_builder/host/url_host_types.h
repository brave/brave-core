/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_TYPES_H_

namespace brave_ads {

enum class UrlHostType {
  kAnonymous = 0,
  kAnonymousSearch,
  kGeo,
  kNonAnonymous,
  kStatic
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_TYPES_H_

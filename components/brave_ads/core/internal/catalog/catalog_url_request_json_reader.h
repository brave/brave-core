/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_JSON_READER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_JSON_READER_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct CatalogInfo;

namespace json::reader {

absl::optional<CatalogInfo> ReadCatalog(const std::string& json);

}  // namespace json::reader
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_JSON_READER_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_ISSUERS_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_ISSUERS_INFO_H_

#include <string>

#include "base/values.h"
#include "bat/ads/internal/catalog/catalog_issuer_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

struct CatalogIssuersInfo {
  CatalogIssuersInfo();
  CatalogIssuersInfo(const CatalogIssuersInfo& info);
  ~CatalogIssuersInfo();

  bool operator==(const CatalogIssuersInfo& rhs) const;
  bool operator!=(const CatalogIssuersInfo& rhs) const;

  base::Value ToDictionary() const;

  bool FromDictionary(base::Value* dictionary);

  bool IsValid() const;

  bool PublicKeyExists(const std::string& public_key) const;

  absl::optional<double> GetEstimatedRedemptionValue(
      const std::string& public_key) const;

  std::string public_key;
  CatalogIssuerList issuers;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_ISSUERS_INFO_H_

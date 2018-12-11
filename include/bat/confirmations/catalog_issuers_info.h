/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <vector>

#include "bat/confirmations/export.h"
#include "bat/confirmations/catalog_issuer_info.h"

namespace confirmations {

struct CONFIRMATIONS_EXPORT CatalogIssuersInfo {
  CatalogIssuersInfo();
  CatalogIssuersInfo(const CatalogIssuersInfo& info);
  ~CatalogIssuersInfo();

  std::vector<CatalogIssuerInfo> issuers;
};

}  // namespace confirmations

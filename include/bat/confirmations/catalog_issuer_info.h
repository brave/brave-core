/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "bat/confirmations/export.h"

namespace confirmations {

struct CONFIRMATIONS_EXPORT CatalogIssuerInfo {
  CatalogIssuerInfo();
  CatalogIssuerInfo(const CatalogIssuerInfo& info);
  ~CatalogIssuerInfo();

  std::string name;
  std::string public_key;
};

}  // namespace confirmations

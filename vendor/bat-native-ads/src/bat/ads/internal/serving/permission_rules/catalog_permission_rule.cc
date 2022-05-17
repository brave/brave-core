/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/catalog_permission_rule.h"

#include "bat/ads/internal/ad_server/catalog/catalog_util.h"

namespace ads {

CatalogPermissionRule::CatalogPermissionRule() = default;

CatalogPermissionRule::~CatalogPermissionRule() = default;

bool CatalogPermissionRule::ShouldAllow() {
  return DoesRespectCap();
}

std::string CatalogPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool CatalogPermissionRule::DoesRespectCap() {
  if (!DoesCatalogExist()) {
    last_message_ = "Catalog does not exist";
    return false;
  }

  if (HasCatalogExpired()) {
    last_message_ = "Catalog has expired";
    return false;
  }

  return true;
}

}  // namespace ads

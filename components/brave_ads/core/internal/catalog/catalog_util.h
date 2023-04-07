/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_UTIL_H_

#include <string>

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct CatalogInfo;

void SaveCatalog(const CatalogInfo& catalog);
void ResetCatalog();

std::string GetCatalogId();
void SetCatalogId(const std::string& id);

int GetCatalogVersion();
void SetCatalogVersion(int version);

base::TimeDelta GetCatalogPing();
void SetCatalogPing(base::TimeDelta ping);

base::Time GetCatalogLastUpdated();
void SetCatalogLastUpdated(base::Time last_updated_at);

bool DoesCatalogExist();
bool HasCatalogChanged(const std::string& catalog_id);
bool HasCatalogExpired();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_UTIL_H_

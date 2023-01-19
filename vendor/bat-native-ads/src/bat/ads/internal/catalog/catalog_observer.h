/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

struct CatalogInfo;

class CatalogObserver : public base::CheckedObserver {
 public:
  // Invoked when the catalog has updated.
  virtual void OnDidUpdateCatalog(const CatalogInfo& catalog) {}

  // Invoked when the catalog failes to update.
  virtual void OnFailedToUpdateCatalog() {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_OBSERVER_H_

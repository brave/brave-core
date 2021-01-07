/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_OBSERVER_H_
#define BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_OBSERVER_H_

#include "base/observer_list.h"

namespace ads {

class Catalog;

class AdServerObserver : public base::CheckedObserver {
 public:
  // Invoked when the catalog has been updated
  virtual void OnCatalogUpdated(
      const Catalog& catalog) {}

  // Invoked when fetching the catalog fails
  virtual void OnCatalogFailed() {}

 protected:
  ~AdServerObserver() override = default;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_OBSERVER_H_

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_CATALOG_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_CATALOG_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using brave_component_updater::DATFileDataBuffer;

namespace brave_shields {

class AdBlockRegionalCatalogProvider {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnRegionalCatalogLoaded(const std::string& catalog_json) = 0;
  };

  AdBlockRegionalCatalogProvider();
  virtual ~AdBlockRegionalCatalogProvider();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  virtual void LoadRegionalCatalog(
      base::OnceCallback<void(const std::string& catalog_json)>) = 0;

 protected:
  void OnRegionalCatalogLoaded(const std::string& catalog_json);

 private:
  base::ObserverList<Observer> observers_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_CATALOG_PROVIDER_H_

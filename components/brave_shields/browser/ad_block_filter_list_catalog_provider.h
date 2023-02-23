/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTER_LIST_CATALOG_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTER_LIST_CATALOG_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace base {
class FilePath;
}

namespace brave_shields {

class AdBlockFilterListCatalogProvider {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFilterListCatalogLoaded(const std::string& catalog_json) = 0;
  };

  explicit AdBlockFilterListCatalogProvider(
      component_updater::ComponentUpdateService* cus);
  ~AdBlockFilterListCatalogProvider();
  AdBlockFilterListCatalogProvider(const AdBlockFilterListCatalogProvider&) =
      delete;
  AdBlockFilterListCatalogProvider& operator=(
      const AdBlockFilterListCatalogProvider&) = delete;

  void LoadFilterListCatalog(
      base::OnceCallback<void(const std::string& catalog_json)>);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void OnFilterListCatalogLoaded(const std::string& catalog_json);
  void OnComponentReady(const base::FilePath&);

  base::FilePath component_path_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<AdBlockFilterListCatalogProvider> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_FILTER_LIST_CATALOG_PROVIDER_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/ad_block_component_filters_provider.h"
#include "brave/components/brave_shields/browser/ad_block_filter_list_catalog_provider.h"
#include "components/prefs/pref_service.h"

class AdBlockServiceTest;

namespace brave_shields {

class FilterListCatalogEntry;

// The AdBlock regional service manager, in charge of initializing and
// managing regional AdBlock clients.
class AdBlockRegionalServiceManager
    : public AdBlockFilterListCatalogProvider::Observer {
 public:
  explicit AdBlockRegionalServiceManager(
      PrefService* local_state,
      std::string locale,
      component_updater::ComponentUpdateService* cus,
      AdBlockFilterListCatalogProvider* catalog_provider);
  AdBlockRegionalServiceManager(const AdBlockRegionalServiceManager&) = delete;
  AdBlockRegionalServiceManager& operator=(
      const AdBlockRegionalServiceManager&) = delete;
  ~AdBlockRegionalServiceManager() override;

  base::Value::List GetRegionalLists();

  bool NeedsLocaleListsMigration(
      std::vector<std::reference_wrapper<FilterListCatalogEntry const>>
          locale_lists);

  void SetFilterListCatalog(std::vector<FilterListCatalogEntry> catalog);
  const std::vector<FilterListCatalogEntry>& GetFilterListCatalog();

  bool IsFilterListAvailable(const std::string& uuid) const;
  bool IsFilterListEnabled(const std::string& uuid) const;
  void EnableFilterList(const std::string& uuid, bool enabled);

  // AdBlockFilterListCatalogProvider::Observer
  void OnFilterListCatalogLoaded(const std::string& catalog_json) override;

 private:
  friend class ::AdBlockServiceTest;
  void StartRegionalServices();
  void UpdateFilterListPrefs(const std::string& uuid, bool enabled);

  void RecordP3ACookieListEnabled();

  // for tests
  const std::map<std::string, std::unique_ptr<AdBlockComponentFiltersProvider>>&
  regional_filters_providers() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return regional_filters_providers_;
  }

  raw_ptr<PrefService> local_state_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::string locale_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::map<std::string, std::unique_ptr<AdBlockComponentFiltersProvider>>
      regional_filters_providers_ GUARDED_BY_CONTEXT(sequence_checker_);

  std::vector<FilterListCatalogEntry> filter_list_catalog_
      GUARDED_BY_CONTEXT(sequence_checker_);

  raw_ptr<component_updater::ComponentUpdateService> component_update_service_
      GUARDED_BY_CONTEXT(sequence_checker_);
  raw_ptr<AdBlockFilterListCatalogProvider> catalog_provider_
      GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AdBlockRegionalServiceManager> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

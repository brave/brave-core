// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Foundation/Foundation.h>

#include "base/apple/foundation_util.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/cosmetic_filters/resources/grit/cosmetic_filters_generated.h"
#include "brave/ios/browser/api/brave_shields/adblock_filter_list_catalog_entry+private.h"
#include "brave/ios/browser/api/brave_shields/adblock_service+private.h"
#include "components/application_locale_storage/application_locale_storage.h"
#include "components/component_updater/component_updater_service.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_shields {
using OnFilterListUpdatedCallback =
    base::RepeatingCallback<void(bool is_default_engine)>;
using OnFilterListCatalogLoadedCallback = base::RepeatingCallback<void()>;
using OnResourceUpdatedCallback =
    base::RepeatingCallback<void(const std::string& resources_json)>;

/// This class listens to changes in the adblock filters provider and notifies
class AdBlockServiceObserver : public AdBlockFiltersProvider::Observer {
 public:
  explicit AdBlockServiceObserver(OnFilterListUpdatedCallback updated_callback);

  // AdBlockFiltersProvider::Observer
  void OnChanged(bool is_default_engine) override;

 private:
  OnFilterListUpdatedCallback updated_callback_;
};

AdBlockServiceObserver::AdBlockServiceObserver(
    OnFilterListUpdatedCallback updated_callback)
    : updated_callback_(updated_callback) {}

void AdBlockServiceObserver::OnChanged(bool is_default_engine) {
  updated_callback_.Run(is_default_engine);
}

class AdBlockCatalogObserver
    : public AdBlockFilterListCatalogProvider::Observer {
 public:
  explicit AdBlockCatalogObserver(OnFilterListCatalogLoadedCallback callback);

  // AdBlockFilterListCatalogProvider::Observer
  void OnFilterListCatalogLoaded(const std::string& catalog_json) override;

 private:
  OnFilterListCatalogLoadedCallback callback_;
};

AdBlockCatalogObserver::AdBlockCatalogObserver(
    OnFilterListCatalogLoadedCallback callback)
    : callback_(callback) {}

void AdBlockCatalogObserver::OnFilterListCatalogLoaded(
    const std::string& catalog_json) {
  callback_.Run();
}

class AdBlockResourceObserver : public AdBlockResourceProvider::Observer {
 public:
  explicit AdBlockResourceObserver(OnResourceUpdatedCallback callback);

  // AdBlockFilterListCatalogProvider::Observer
  void OnResourcesLoaded(const std::string& resources_json) override;

 private:
  OnResourceUpdatedCallback callback_;
};

AdBlockResourceObserver::AdBlockResourceObserver(
    OnResourceUpdatedCallback callback)
    : callback_(callback) {}

void AdBlockResourceObserver::OnResourcesLoaded(
    const std::string& resources_json) {
  callback_.Run(resources_json);
}
}  // namespace brave_shields

@interface AdblockService () {
  raw_ptr<component_updater::ComponentUpdateService> _cus;  // NOT OWNED
  std::unique_ptr<brave_shields::AdBlockListP3A> _adblockListP3A;
  std::unique_ptr<brave_shields::AdBlockFilterListCatalogProvider>
      _catalogProvider;
  std::unique_ptr<brave_shields::AdBlockFiltersProviderManager>
      _filtersProviderManager;
  std::unique_ptr<brave_shields::AdBlockComponentServiceManager>
      _serviceManager;
  std::unique_ptr<brave_shields::AdBlockDefaultResourceProvider>
      _resourceProvider;
  std::vector<std::unique_ptr<brave_shields::AdBlockServiceObserver>>
      _serviceObservers;
  std::unique_ptr<brave_shields::AdBlockCatalogObserver> _catalogObserver;
  std::unique_ptr<brave_shields::AdBlockResourceObserver> _resourceObserver;
}

@property(nonatomic)
    NSArray<AdblockFilterListCatalogEntry*>* filterListCatalogEntries;
@property(nonatomic) NSURL* resourcesPath;
@end

@implementation AdblockService

- (instancetype)initWithComponentUpdater:
    (component_updater::ComponentUpdateService*)componentUpdaterService {
  if ((self = [super init])) {
    _cus = componentUpdaterService;
    _adblockListP3A = std::make_unique<brave_shields::AdBlockListP3A>(
        GetApplicationContext()->GetLocalState());
    _catalogProvider =
        std::make_unique<brave_shields::AdBlockFilterListCatalogProvider>(_cus);
    _filtersProviderManager =
        std::make_unique<brave_shields::AdBlockFiltersProviderManager>();
    _serviceManager =
        std::make_unique<brave_shields::AdBlockComponentServiceManager>(
            GetApplicationContext()->GetLocalState(),
            _filtersProviderManager.get(),
            GetApplicationContext()->GetApplicationLocaleStorage()->Get(), _cus,
            _catalogProvider.get(), _adblockListP3A.get());
    _resourceProvider =
        std::make_unique<brave_shields::AdBlockDefaultResourceProvider>(_cus);
  }
  return self;
}

- (void)registerFilterListChanges:(void (^)(bool isDefaultEngine))callback {
  auto _serviceObserver =
      std::make_unique<brave_shields::AdBlockServiceObserver>(
          base::BindRepeating(callback));
  _filtersProviderManager->AddObserver(_serviceObserver.get());
  _serviceObservers.push_back(std::move(_serviceObserver));
}

- (void)registerCatalogChanges:(void (^)())callback {
  _catalogObserver = std::make_unique<brave_shields::AdBlockCatalogObserver>(
      base::BindRepeating(callback));
  _catalogProvider->AddObserver(_catalogObserver.get());
}

- (void)registerResourcesChanges:(void (^)(NSString* resourcesJSON))callback {
  _resourceObserver = std::make_unique<brave_shields::AdBlockResourceObserver>(
      base::BindRepeating(^(const std::string& resources_json) {
        const auto resourcesJSON = base::SysUTF8ToNSString(resources_json);
        callback(resourcesJSON);
      }));
  _resourceProvider->AddObserver(_resourceObserver.get());
}

- (NSArray<AdblockFilterListCatalogEntry*>*)filterListCatalogEntries {
  auto catalog = _serviceManager->GetFilterListCatalog();
  NSMutableArray* lists = [[NSMutableArray alloc] init];

  for (const auto& entry : catalog) {
    if (entry.SupportsCurrentPlatform()) {
      [lists addObject:[[AdblockFilterListCatalogEntry alloc]
                           initWithFilterListCatalogEntry:
                               brave_shields::FilterListCatalogEntry(entry)]];
    }
  }

  return lists;
}

- (NSURL*)resourcesPath {
  _resourceProvider->GetResourcesPath();
  base::FilePath file_path = _resourceProvider->GetResourcesPath();
  if (file_path.empty()) {
    return nil;
  }
  return base::apple::FilePathToNSURL(file_path);
}

- (NSURL*)installPathForFilterListUUID:(NSString*)uuid {
  base::FilePath file_path =
      _serviceManager->GetFilterSetPath(base::SysNSStringToUTF8(uuid));
  if (file_path.empty()) {
    return nil;
  }
  return base::apple::FilePathToNSURL(file_path);
}

- (void)enableFilterListForUUID:(NSString*)uuid isEnabled:(bool)isEnabled {
  _serviceManager->EnableFilterList(base::SysNSStringToUTF8(uuid), isEnabled);
}

- (bool)isFilterListAvailableForUUID:(NSString*)uuid {
  return _serviceManager->IsFilterListAvailable(base::SysNSStringToUTF8(uuid));
}

- (bool)isFilterListEnabledForUUID:(NSString*)uuid {
  return _serviceManager->IsFilterListEnabled(base::SysNSStringToUTF8(uuid));
}

- (void)updateFilterLists:(void (^)(bool))callback {
  _serviceManager->UpdateFilterLists(base::BindOnce(callback));
}

+ (NSString*)cosmeticFiltersScript {
  // The resource bundle is not available until after WebMainParts is setup
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::string resource_string = "";
  if (resource_bundle.IsGzipped(
          IDR_COSMETIC_FILTERS_CONTENT_COSMETIC_IOS_BUNDLE_JS)) {
    resource_string = std::string(resource_bundle.LoadDataResourceString(
        IDR_COSMETIC_FILTERS_CONTENT_COSMETIC_IOS_BUNDLE_JS));
  } else {
    resource_string = std::string(resource_bundle.GetRawDataResource(
        IDR_COSMETIC_FILTERS_CONTENT_COSMETIC_IOS_BUNDLE_JS));
  }
  return base::SysUTF8ToNSString(resource_string);
}

@end

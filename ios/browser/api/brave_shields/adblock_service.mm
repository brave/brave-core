// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Foundation/Foundation.h>

#include "base/apple/foundation_util.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
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
#include "brave/components/brave_shields/core/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/cosmetic_filters/common/scriptlet_constants.h"
#include "brave/components/cosmetic_filters/resources/grit/cosmetic_filters_generated.h"
#include "brave/ios/browser/api/brave_shields/adblock_filter_list_catalog_entry+private.h"
#include "brave/ios/browser/api/brave_shields/adblock_service+private.h"
#include "brave/ios/browser/brave_shields/ad_block_service_factory.h"
#include "components/application_locale_storage/application_locale_storage.h"
#include "components/component_updater/component_updater_service.h"
#include "components/grit/brave_components_resources.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_shields {
using OnFilterListUpdatedCallback =
    base::RepeatingCallback<void(bool is_default_engine)>;
using OnFilterListCatalogLoadedCallback = base::RepeatingCallback<void()>;
using OnResourceUpdatedCallback = base::RepeatingCallback<void()>;

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
  void OnResourcesLoaded(AdblockResourceStorageBox) override;

 private:
  OnResourceUpdatedCallback callback_;
};

AdBlockResourceObserver::AdBlockResourceObserver(
    OnResourceUpdatedCallback callback)
    : callback_(callback) {}

void AdBlockResourceObserver::OnResourcesLoaded(
    AdblockResourceStorageBox /*storage*/) {
  callback_.Run();
}

}  // namespace brave_shields

@interface AdblockService () {
  raw_ptr<component_updater::ComponentUpdateService> _cus;  // NOT OWNED
  // Only owned when the shared AdBlockService is disabled.
  std::unique_ptr<brave_shields::AdBlockListP3A> _ownedAdblockListP3A;
  std::unique_ptr<brave_shields::AdBlockFilterListCatalogProvider>
      _ownedCatalogProvider;
  std::unique_ptr<brave_shields::AdBlockFiltersProviderManager>
      _ownedFiltersProviderManager;
  std::unique_ptr<brave_shields::AdBlockComponentServiceManager>
      _ownedServiceManager;
  std::unique_ptr<brave_shields::AdBlockDefaultResourceProvider>
      _ownedResourceProvider;

  raw_ptr<brave_shields::AdBlockFilterListCatalogProvider> _catalogProvider;
  raw_ptr<brave_shields::AdBlockFiltersProviderManager> _filtersProviderManager;
  raw_ptr<brave_shields::AdBlockComponentServiceManager> _serviceManager;
  raw_ptr<brave_shields::AdBlockDefaultResourceProvider> _resourceProvider;

  std::vector<std::unique_ptr<brave_shields::AdBlockServiceObserver>>
      _serviceObservers;
  std::unique_ptr<brave_shields::AdBlockCatalogObserver> _catalogObserver;
  std::unique_ptr<brave_shields::AdBlockResourceObserver> _resourceObserver;

  // Calls made before the shared AdBlockService is attached.
  base::flat_map<std::string, bool> _pendingFilterListStates;
  std::vector<base::OnceCallback<void(bool)>> _pendingUpdateCallbacks;
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
    // The shared AdBlockService is profile-keyed and is attached in
    // `-attachToProfile:`.
    if (!base::FeatureList::IsEnabled(
            brave_shields::features::kBraveIOSSharedAdBlockService)) {
      _ownedAdblockListP3A = std::make_unique<brave_shields::AdBlockListP3A>(
          GetApplicationContext()->GetLocalState());
      _ownedCatalogProvider =
          std::make_unique<brave_shields::AdBlockFilterListCatalogProvider>(
              _cus);
      _ownedFiltersProviderManager =
          std::make_unique<brave_shields::AdBlockFiltersProviderManager>();
      _ownedServiceManager =
          std::make_unique<brave_shields::AdBlockComponentServiceManager>(
              GetApplicationContext()->GetLocalState(),
              _ownedFiltersProviderManager.get(),
              std::string(GetApplicationContext()
                              ->GetApplicationLocaleStorage()
                              ->GetTag()
                              .tag_string()),
              _cus, _ownedCatalogProvider.get(), _ownedAdblockListP3A.get());
      _ownedResourceProvider =
          std::make_unique<brave_shields::AdBlockDefaultResourceProvider>(_cus);

      _catalogProvider = _ownedCatalogProvider.get();
      _filtersProviderManager = _ownedFiltersProviderManager.get();
      _serviceManager = _ownedServiceManager.get();
      _resourceProvider = _ownedResourceProvider.get();
    }
  }
  return self;
}

- (void)attachToProfile:(ProfileIOS*)profile {
  brave_shields::AdBlockService* adBlockService =
      brave_shields::AdBlockServiceFactory::GetForProfile(profile);
  if (!adBlockService || _serviceManager) {
    return;
  }
  _catalogProvider = adBlockService->filter_list_catalog_provider();
  _filtersProviderManager = adBlockService->filters_provider_manager();
  _serviceManager = adBlockService->component_service_manager();
  _resourceProvider = adBlockService->default_resource_provider();

  for (const auto& observer : _serviceObservers) {
    _filtersProviderManager->AddObserver(observer.get());
  }
  if (_catalogObserver) {
    _catalogProvider->AddObserver(_catalogObserver.get());
  }
  if (_resourceObserver) {
    _resourceProvider->AddObserver(_resourceObserver.get());
  }

  for (const auto& [uuid, enabled] : _pendingFilterListStates) {
    _serviceManager->EnableFilterList(uuid, enabled);
  }
  _pendingFilterListStates.clear();
  for (auto& callback : _pendingUpdateCallbacks) {
    _serviceManager->UpdateFilterLists(std::move(callback));
  }
  _pendingUpdateCallbacks.clear();
}

- (void)detachFromProfile {
  if (_ownedServiceManager || !_serviceManager) {
    return;
  }
  for (const auto& observer : _serviceObservers) {
    _filtersProviderManager->RemoveObserver(observer.get());
  }
  if (_catalogObserver) {
    _catalogProvider->RemoveObserver(_catalogObserver.get());
  }
  if (_resourceObserver) {
    _resourceProvider->RemoveObserver(_resourceObserver.get());
  }
  _catalogProvider = nullptr;
  _filtersProviderManager = nullptr;
  _serviceManager = nullptr;
  _resourceProvider = nullptr;
}

- (void)registerFilterListChanges:(void (^)(bool isDefaultEngine))callback {
  auto _serviceObserver =
      std::make_unique<brave_shields::AdBlockServiceObserver>(
          base::BindRepeating(callback));
  if (_filtersProviderManager) {
    _filtersProviderManager->AddObserver(_serviceObserver.get());
  }
  _serviceObservers.push_back(std::move(_serviceObserver));
}

- (void)registerCatalogChanges:(void (^)())callback {
  _catalogObserver = std::make_unique<brave_shields::AdBlockCatalogObserver>(
      base::BindRepeating(callback));
  if (_catalogProvider) {
    _catalogProvider->AddObserver(_catalogObserver.get());
  }
}

- (void)registerResourcesChanges:(void (^)())callback {
  _resourceObserver = std::make_unique<brave_shields::AdBlockResourceObserver>(
      base::BindRepeating(callback));
  if (_resourceProvider) {
    _resourceProvider->AddObserver(_resourceObserver.get());
  }
}

- (NSArray<AdblockFilterListCatalogEntry*>*)filterListCatalogEntries {
  if (!_serviceManager) {
    return @[];
  }
  auto catalog = _serviceManager->GetFilterListCatalog();
  NSMutableArray* lists = [[NSMutableArray alloc] init];

  for (const auto& entry : catalog) {
    if (entry.SupportsCurrentPlatform() && entry.public_key_sha256) {
      [lists addObject:[[AdblockFilterListCatalogEntry alloc]
                           initWithFilterListCatalogEntry:
                               brave_shields::FilterListCatalogEntry(entry)]];
    }
  }

  return lists;
}

- (NSURL*)resourcesPath {
  if (!_resourceProvider) {
    return nil;
  }
  base::FilePath file_path = _resourceProvider->GetResourcesPath();
  if (file_path.empty()) {
    return nil;
  }
  return base::apple::FilePathToNSURL(file_path);
}

- (NSURL*)installPathForFilterListUUID:(NSString*)uuid {
  if (!_serviceManager) {
    return nil;
  }
  base::FilePath file_path =
      _serviceManager->GetFilterSetPath(base::SysNSStringToUTF8(uuid));
  if (file_path.empty()) {
    return nil;
  }
  return base::apple::FilePathToNSURL(file_path);
}

- (void)enableFilterListForUUID:(NSString*)uuid isEnabled:(bool)isEnabled {
  if (!_serviceManager) {
    _pendingFilterListStates[base::SysNSStringToUTF8(uuid)] = isEnabled;
    return;
  }
  _serviceManager->EnableFilterList(base::SysNSStringToUTF8(uuid), isEnabled);
}

- (bool)isFilterListAvailableForUUID:(NSString*)uuid {
  if (!_serviceManager) {
    return false;
  }
  return _serviceManager->IsFilterListAvailable(base::SysNSStringToUTF8(uuid));
}

- (bool)isFilterListEnabledForUUID:(NSString*)uuid {
  if (!_serviceManager) {
    const auto it =
        _pendingFilterListStates.find(base::SysNSStringToUTF8(uuid));
    return it != _pendingFilterListStates.end() && it->second;
  }
  return _serviceManager->IsFilterListEnabled(base::SysNSStringToUTF8(uuid));
}

- (void)updateFilterLists:(void (^)(bool))callback {
  if (!_serviceManager) {
    _pendingUpdateCallbacks.push_back(base::BindOnce(callback));
    return;
  }
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

+ (NSString*)scriptletGlobalsScriptWithDeAmpEnabled:(bool)isDeAmpEnabled {
  // iOS doesn't support the canDebug flag, so just keep as false.
  return base::SysUTF8ToNSString(cosmetic_filters::GetScriptletGlobalsScript(
      isDeAmpEnabled, /*can_debug=*/false));
}

@end

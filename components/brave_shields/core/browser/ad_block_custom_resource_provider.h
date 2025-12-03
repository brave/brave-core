// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_CUSTOM_RESOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_CUSTOM_RESOURCE_PROVIDER_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"

namespace value_store {
class ValueStoreFrontend;
}  // namespace value_store

class PrefService;

namespace brave_shields {

class AdBlockCustomResourceProvider
    : public AdBlockResourceProvider,
      private AdBlockResourceProvider::Observer {
 public:
  enum class ErrorCode {
    kOk,
    kInvalid,
    kAlreadyExists,
    kNotFound,
  };

  struct Observer : public base::CheckedObserver {
    ~Observer() override = default;
    virtual void OnCustomResourcesChanged() {}
  };

  using GetCallback = base::OnceCallback<void(base::Value)>;
  using StatusCallback = base::OnceCallback<void(ErrorCode)>;

  AdBlockCustomResourceProvider(
      const base::FilePath& storage_root,
      std::unique_ptr<AdBlockResourceProvider> default_resource_provider);
  ~AdBlockCustomResourceProvider() override;

  void GetCustomResources(GetCallback callback);
  void AddResource(PrefService* profile_prefs,
                   const base::Value& resource,
                   StatusCallback on_complete);
  void UpdateResource(PrefService* profile_prefs,
                      const std::string& name,
                      const base::Value& resource,
                      StatusCallback on_complete);
  void RemoveResource(PrefService* profile_prefs,
                      const std::string& resource_name,
                      StatusCallback on_complete);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // AdBlockResourceProvider:
  void LoadResources(
      base::OnceCallback<void(AdblockResourceStorageBox)>) override;

 private:
  // AdBlockResourceProvider::Observer:
  void OnResourcesLoaded(AdblockResourceStorageBox) override;

  void AddResourceInternal(base::Value resource,
                           StatusCallback on_complete,
                           base::Value resources);
  void UpdateResourceInternal(const std::string& name,
                              base::Value resource,
                              StatusCallback on_complete,
                              base::Value resources);
  void RemoveResourceInternal(const std::string& name,
                              StatusCallback on_complete,
                              base::Value resources);

  void SaveResources(base::Value resources);

  void OnDefaultResourcesLoaded(
      base::OnceCallback<void(AdblockResourceStorageBox)> on_load,
      AdblockResourceStorageBox storage);
  void OnCustomResourcesLoaded(
      base::OnceCallback<void(AdblockResourceStorageBox)> on_load,
      AdblockResourceStorageBox default_storage,
      base::Value custom_resources);

  void ReloadResourcesAndNotify();

  std::unique_ptr<AdBlockResourceProvider> default_resource_provider_ = nullptr;
  std::unique_ptr<value_store::ValueStoreFrontend> storage_;
  base::ObserverList<Observer> observers_;
  std::optional<AdblockResourceStorageBox> cached_storage_ = std::nullopt;

  base::WeakPtrFactory<AdBlockCustomResourceProvider> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_CUSTOM_RESOURCE_PROVIDER_H_

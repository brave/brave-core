// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_CUSTOM_RESOURCE_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_CUSTOM_RESOURCE_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"

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

  AdBlockCustomResourceProvider(
      PrefService* local_state,
      std::unique_ptr<AdBlockResourceProvider> default_resource_provider);
  ~AdBlockCustomResourceProvider() override;

  const base::Value& GetCustomResources();
  ErrorCode AddResource(const base::Value& resource);
  ErrorCode UpdateResource(const std::string& name,
                           const base::Value& resource);
  ErrorCode RemoveResource(const std::string& resource_name);

  // AdBlockResourceProvider:
  void LoadResources(
      base::OnceCallback<void(const std::string& resources_json)>) override;

 private:
  // AdBlockResourceProvider::Observer:
  void OnResourcesLoaded(const std::string& resources_json) override;

  std::string GetCustomResourcesJson();
  void OnDefaultResourcesLoaded(
      base::OnceCallback<void(const std::string& resources_json)> on_load,
      const std::string& resources_json);
  void ReloadResourcesAndNotify();

  const raw_ptr<PrefService> local_state_ = nullptr;
  std::unique_ptr<AdBlockResourceProvider> default_resource_provider_ = nullptr;

  base::WeakPtrFactory<AdBlockCustomResourceProvider> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_CUSTOM_RESOURCE_PROVIDER_H_

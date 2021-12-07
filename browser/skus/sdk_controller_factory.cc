// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/skus/sdk_controller_factory.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/skus/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace {

bool IsAllowedForContext(content::BrowserContext* context) {
  if (context && !brave::IsRegularProfile(context))
    return false;

  return true;
}

}  // namespace

namespace skus {

// static
SdkControllerFactory* SdkControllerFactory::GetInstance() {
  return base::Singleton<SdkControllerFactory>::get();
}

// static
mojo::PendingRemote<mojom::SdkController> SdkControllerFactory::GetForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return mojo::PendingRemote<mojom::SdkController>();
  }
  return static_cast<skus::SdkController*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
skus::SdkController* SdkControllerFactory::GetControllerForContext(
    content::BrowserContext* context) {
  if (!IsAllowedForContext(context)) {
    return nullptr;
  }
  return static_cast<skus::SdkController*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

SdkControllerFactory::SdkControllerFactory()
    : BrowserContextKeyedServiceFactory(
          "SdkController",
          BrowserContextDependencyManager::GetInstance()) {}

SdkControllerFactory::~SdkControllerFactory() = default;

KeyedService* SdkControllerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Skus functionality not supported in private / Tor / guest windows
  if (!brave::IsRegularProfile(context)) {
    return nullptr;
  }

  return new skus::SdkController(
      Profile::FromBrowserContext(context)->GetPrefs(),
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess());
}

void SdkControllerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSkusState);
  registry->RegisterBooleanPref(prefs::kSkusVPNHasCredential, false);
}

}  // namespace skus

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/cookies/brave_cookies_api_helpers.h"

#include <map>
#include <utility>

#include "base/feature_list.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/extensions/api/cookies/cookies_helpers.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/window_controller.h"
#include "chrome/browser/extensions/window_controller_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/cookies.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/error_utils.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/containers/cookie_store_id.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "content/public/browser/storage_partition_config.h"
#endif

namespace extensions {
namespace brave_cookies_api_helpers {
namespace {

constexpr char kInvalidStoreIdError[] = "Invalid cookie store id: \"*\".";

#if BUILDFLAG(ENABLE_CONTAINERS)
network::mojom::CookieManager* GetContainerCookieManager(
    Profile* profile,
    const std::string& container_id,
    std::string* error) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    *error = ErrorUtils::FormatErrorMessage(kInvalidStoreIdError,
                                            containers::GetContainerStoreId(
                                                container_id));
    return nullptr;
  }

  containers::ContainersService* service =
      ContainersServiceFactory::GetForProfile(profile);
  if (!service || !service->GetRuntimeContainerById(container_id)) {
    *error = ErrorUtils::FormatErrorMessage(
        kInvalidStoreIdError, containers::GetContainerStoreId(container_id));
    return nullptr;
  }

  const auto config = content::StoragePartitionConfig::Create(
      profile, containers::kContainersStoragePartitionDomain, container_id,
      profile->IsOffTheRecord());
  return profile->GetStoragePartition(config)
      ->GetCookieManagerForBrowserProcess();
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

api::cookies::CookieStore CreateCookieStoreForId(const std::string& store_id,
                                                 base::ListValue tab_ids) {
  base::DictValue dict;
  dict.Set("id", store_id);
  dict.Set("tabIds", std::move(tab_ids));
  auto cookie_store = api::cookies::CookieStore::FromValue(dict);
  CHECK(cookie_store);
  return std::move(cookie_store).value();
}

}  // namespace

network::mojom::CookieManager* ParseStoreCookieManager(
    content::BrowserContext* function_context,
    bool include_incognito,
    std::string* store_id,
    std::string* error) {
  Profile* function_profile = Profile::FromBrowserContext(function_context);

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (!store_id->empty()) {
    if (auto container_id = containers::ParseContainerStoreId(*store_id)) {
      return GetContainerCookieManager(function_profile, *container_id, error);
    }
  }
#endif

  Profile* store_profile = nullptr;
  if (!store_id->empty()) {
    store_profile = cookies_helpers::ChooseProfileFromStoreId(
        *store_id, function_profile, include_incognito);
    if (!store_profile) {
      *error = ErrorUtils::FormatErrorMessage(kInvalidStoreIdError, *store_id);
      return nullptr;
    }
  } else {
    store_profile = function_profile;
    *store_id = cookies_helpers::GetStoreIdFromProfile(store_profile);
  }

  return store_profile->GetDefaultStoragePartition()
      ->GetCookieManagerForBrowserProcess();
}

std::vector<api::cookies::CookieStore> GetAllCookieStores(
    content::BrowserContext* browser_context,
    bool include_incognito) {
  Profile* original_profile = Profile::FromBrowserContext(browser_context);
  DCHECK(original_profile);

  Profile* incognito_profile = nullptr;
  if (include_incognito && original_profile->HasPrimaryOTRProfile()) {
    incognito_profile =
        original_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  }
  DCHECK(original_profile != incognito_profile);

  // store_id -> tab ids
  std::map<std::string, base::ListValue> tabs_by_store;

  for (WindowController* window : *WindowControllerList::GetInstance()) {
    if (window->profile() != original_profile &&
        window->profile() != incognito_profile) {
      continue;
    }

    for (int i = 0; i < window->GetTabCount(); ++i) {
      content::WebContents* contents = window->GetWebContentsAt(i);
      if (!contents) {
        continue;
      }
      std::string store_id;
#if BUILDFLAG(ENABLE_CONTAINERS)
      if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
        store_id = containers::GetStoreIdForWebContents(contents);
      } else {
        store_id = cookies_helpers::GetStoreIdFromProfile(window->profile());
      }
#else
      store_id = cookies_helpers::GetStoreIdFromProfile(window->profile());
#endif
      tabs_by_store[store_id].Append(ExtensionTabUtil::GetTabId(contents));
    }
  }

  std::vector<api::cookies::CookieStore> cookie_stores;
  for (auto& [store_id, tab_ids] : tabs_by_store) {
    if (tab_ids.empty()) {
      continue;
    }
    cookie_stores.push_back(
        CreateCookieStoreForId(store_id, std::move(tab_ids)));
  }

  return cookie_stores;
}

}  // namespace brave_cookies_api_helpers
}  // namespace extensions

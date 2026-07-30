/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/api/cookies/cookies_api.h"

#include "brave/browser/extensions/api/cookies/brave_cookies_api_helpers.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/storage_partition.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "base/containers/flat_set.h"
#include "brave/browser/containers/cookie_store_id.h"
#include "brave/browser/containers/used_container_storage_partitions.h"
#include "brave/components/containers/core/browser/pref_names.h"
#include "brave/components/containers/core/common/features.h"
#include "components/prefs/pref_service.h"
#endif

// This disables cookies.onChange routing in Tor windows (as it worked
// when there was a DCHECK instead of CHECK). Once crbug.com/417228685 is fixed
// in upstream, we'll be able to manage Tor profiles like any other main-OTR
// profile.
//
// `store_id == "1"` is the OTR cookie store (was the `otr` bool before Brave
// generalized OnCookieChange to carry opaque store ids for containers).

#define IsSerializeable(...)                             \
  IsSerializeable(__VA_ARGS__) ||                        \
      (store_id == "1" &&                                \
       !profile_->GetPrimaryOTRProfile(/*create_if_needed=*/false))

#define OnOffTheRecordProfileCreated(...) \
  OnOffTheRecordProfileCreated_ChromiumImpl(__VA_ARGS__)

#include "brave/browser/extensions/api/cookies/brave_cookies_api_helpers.cc"
#include <chrome/browser/extensions/api/cookies/cookies_api.cc>

#undef IsSerializeable
#undef OnOffTheRecordProfileCreated

namespace extensions {

// static
void OnCookieChangeExposeForTesting::CallOnCookieChangeForOtr(
    CookiesAPI* cookies_api) {
  cookies_api->cookies_event_router_->OnCookieChange("1",
                                                     net::CookieChangeInfo());
}

void CookiesEventRouter::OnOffTheRecordProfileCreated(Profile* off_the_record) {
  if (off_the_record->IsTor()) {
    return;
  }

  OnOffTheRecordProfileCreated_ChromiumImpl(off_the_record);
}

CookiesEventRouter::ContainerListenerState::ContainerListenerState(
    CookiesEventRouter* router,
    const std::string& store_id)
    : listener(router, store_id), receiver(&listener) {}

CookiesEventRouter::ContainerListenerState::~ContainerListenerState() = default;

void CookiesEventRouter::SyncContainerCookieListeners() {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    container_listeners_.clear();
    return;
  }

  if (pref_change_registrar_.IsEmpty() && profile_->GetPrefs()) {
    pref_change_registrar_.Init(profile_->GetPrefs());
    pref_change_registrar_.Add(
        containers::prefs::kLocallyUsedContainers,
        base::BindRepeating(&CookiesEventRouter::SyncContainerCookieListeners,
                            base::Unretained(this)));
  }

  const auto configs =
      containers::GetUsedContainerStoragePartitionConfigs(profile_);
  base::flat_set<std::string> wanted_store_ids;
  for (const auto& config : configs) {
    wanted_store_ids.insert(
        containers::GetContainerStoreId(config.partition_name()));
  }

  for (auto it = container_listeners_.begin();
       it != container_listeners_.end();) {
    if (!wanted_store_ids.contains(it->first)) {
      it = container_listeners_.erase(it);
    } else {
      ++it;
    }
  }

  for (const auto& config : configs) {
    const std::string store_id =
        containers::GetContainerStoreId(config.partition_name());
    if (container_listeners_.contains(store_id)) {
      continue;
    }

    auto state = std::make_unique<ContainerListenerState>(this, store_id);
    network::mojom::CookieManager* cookie_manager =
        profile_->GetStoragePartition(config)
            ->GetCookieManagerForBrowserProcess();
    if (!cookie_manager) {
      continue;
    }
    cookie_manager->AddGlobalChangeListener(
        state->receiver.BindNewPipeAndPassRemote());
    state->receiver.set_disconnect_handler(base::BindOnce(
        [](CookiesEventRouter* router, const std::string& id) {
          router->container_listeners_.erase(id);
          router->SyncContainerCookieListeners();
        },
        base::Unretained(this), store_id));
    container_listeners_.emplace(store_id, std::move(state));
  }
#else
  container_listeners_.clear();
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

}  // namespace extensions

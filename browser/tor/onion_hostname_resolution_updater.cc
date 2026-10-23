/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/onion_hostname_resolution_updater.h"

#include "base/functional/bind.h"
#include "brave/components/tor/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace tor {

network::mojom::TorMode GetNetworkContextTorMode(
    content::BrowserContext* context) {
  if (context->IsTor()) {
    return network::mojom::TorMode::kEnabled;
  }
  return user_prefs::UserPrefs::Get(context)->GetBoolean(
             prefs::kOnionOnlyInTorWindows)
             ? network::mojom::TorMode::kDisabled
             : network::mojom::TorMode::kPassthrough;
}

OnionHostnameResolutionUpdater::OnionHostnameResolutionUpdater(
    content::BrowserContext* context)
    : context_(context) {
  pref_change_registrar_.Init(user_prefs::UserPrefs::Get(context));
  pref_change_registrar_.Add(
      prefs::kOnionOnlyInTorWindows,
      base::BindRepeating(
          &OnionHostnameResolutionUpdater::UpdateNetworkContexts,
          base::Unretained(this)));
}

OnionHostnameResolutionUpdater::~OnionHostnameResolutionUpdater() = default;

void OnionHostnameResolutionUpdater::Shutdown() {
  pref_change_registrar_.RemoveAll();
}

void OnionHostnameResolutionUpdater::UpdateNetworkContexts() {
  const network::mojom::TorMode tor_mode = GetNetworkContextTorMode(context_);
  context_->ForEachLoadedStoragePartition(
      [tor_mode](content::StoragePartition* storage_partition) {
        storage_partition->GetNetworkContext()->SetTorMode(tor_mode);
      });
}

}  // namespace tor

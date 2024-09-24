/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_RENDERER_UPDATER_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_RENDERER_UPDATER_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/common/brave_renderer_configuration.mojom-forward.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_member.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

class Profile;

namespace brave_wallet {
class KeyringService;
}

namespace content {
class RenderProcessHost;
}

class BraveRendererUpdater : public KeyedService {
 public:
  BraveRendererUpdater(Profile* profile,
                       brave_wallet::KeyringService* keyring_service,
                       PrefService* local_state);
  BraveRendererUpdater(const BraveRendererUpdater&) = delete;
  BraveRendererUpdater& operator=(const BraveRendererUpdater&) = delete;
  ~BraveRendererUpdater() override;

  // Initialize a newly-started renderer process.
  void InitializeRenderer(content::RenderProcessHost* render_process_host);

 private:
  std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
  GetRendererConfigurations();

  mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
  GetRendererConfiguration(content::RenderProcessHost* render_process_host);

  // Update renderers if wallet keyring has been initialized
  void CheckActiveWalletAndMaybeUpdateRenderers();

  // Update active wallet bool, returns true if status has changed
  bool CheckActiveWallet();

  // Update all renderers due to a configuration change.
  void UpdateAllRenderers();

  // Update the given renderer due to a configuration change.
  void UpdateRenderer(
      mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>*
          renderer_configuration);

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<brave_wallet::KeyringService> keyring_service_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  PrefChangeRegistrar local_state_change_registrar_;

  // Prefs that we sync to the renderers.
  IntegerPrefMember brave_wallet_ethereum_provider_;
  IntegerPrefMember brave_wallet_solana_provider_;
  BooleanPrefMember de_amp_enabled_;
#if BUILDFLAG(ENABLE_TOR)
  BooleanPrefMember onion_only_in_tor_windows_;
#endif
  BooleanPrefMember widevine_enabled_;
  bool is_wallet_allowed_for_context_ = false;
  bool is_wallet_created_ = false;
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_RENDERER_UPDATER_H_

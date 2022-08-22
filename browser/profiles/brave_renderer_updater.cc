/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_renderer_updater.h"

#include <utility>

#include "base/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/common/brave_renderer_configuration.mojom.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/de_amp/browser/de_amp_util.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "ipc/ipc_channel_proxy.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

BraveRendererUpdater::BraveRendererUpdater(Profile* profile)
    : profile_(profile), is_wallet_allowed_for_context_(false) {
  PrefService* pref_service = profile->GetPrefs();
  brave_wallet_ethereum_provider_.Init(kDefaultEthereumWallet, pref_service);
  brave_wallet_solana_provider_.Init(kDefaultSolanaWallet, pref_service);
  de_amp_enabled_.Init(de_amp::kDeAmpPrefEnabled, pref_service);

  pref_change_registrar_.Init(pref_service);
  pref_change_registrar_.Add(
      kDefaultEthereumWallet,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kDefaultSolanaWallet,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      de_amp::kDeAmpPrefEnabled,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
}

BraveRendererUpdater::~BraveRendererUpdater() = default;

void BraveRendererUpdater::InitializeRenderer(
    content::RenderProcessHost* render_process_host) {
  auto renderer_configuration = GetRendererConfiguration(render_process_host);
  Profile* profile =
      Profile::FromBrowserContext(render_process_host->GetBrowserContext());
  is_wallet_allowed_for_context_ = brave_wallet::IsAllowedForContext(profile);
  UpdateRenderer(&renderer_configuration);
}

std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
BraveRendererUpdater::GetRendererConfigurations() {
  std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
      rv;
  for (content::RenderProcessHost::iterator it(
           content::RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    Profile* renderer_profile =
        static_cast<Profile*>(it.GetCurrentValue()->GetBrowserContext());
    if (renderer_profile == profile_ ||
        renderer_profile->GetOriginalProfile() == profile_) {
      auto renderer_configuration =
          GetRendererConfiguration(it.GetCurrentValue());
      if (renderer_configuration)
        rv.push_back(std::move(renderer_configuration));
    }
  }
  return rv;
}

mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
BraveRendererUpdater::GetRendererConfiguration(
    content::RenderProcessHost* render_process_host) {
  IPC::ChannelProxy* channel = render_process_host->GetChannel();
  if (!channel)
    return mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>();

  mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
      renderer_configuration;
  channel->GetRemoteAssociatedInterface(&renderer_configuration);

  return renderer_configuration;
}

void BraveRendererUpdater::UpdateAllRenderers() {
  auto renderer_configurations = GetRendererConfigurations();
  for (auto& renderer_configuration : renderer_configurations)
    UpdateRenderer(&renderer_configuration);
}

void BraveRendererUpdater::UpdateRenderer(
    mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>*
        renderer_configuration) {
  auto default_ethereum_wallet =
      static_cast<brave_wallet::mojom::DefaultWallet>(
          brave_wallet_ethereum_provider_.GetValue());
  bool brave_use_native_ethereum_wallet =
      (default_ethereum_wallet ==
           brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension ||
       default_ethereum_wallet ==
           brave_wallet::mojom::DefaultWallet::BraveWallet) &&
      is_wallet_allowed_for_context_ && brave_wallet::IsDappsSupportEnabled();
  bool allow_overwrite_window_ethereum_provider =
      default_ethereum_wallet ==
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension;

  auto default_solana_wallet = static_cast<brave_wallet::mojom::DefaultWallet>(
      brave_wallet_solana_provider_.GetValue());
  bool brave_use_native_solana_wallet =
      (default_solana_wallet ==
           brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension ||
       default_solana_wallet ==
           brave_wallet::mojom::DefaultWallet::BraveWallet) &&
      is_wallet_allowed_for_context_ && brave_wallet::IsDappsSupportEnabled();
  bool allow_overwrite_window_solana_provider =
      default_solana_wallet ==
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension;

  PrefService* pref_service = profile_->GetPrefs();
  bool de_amp_enabled = de_amp::IsDeAmpEnabled(pref_service);

  (*renderer_configuration)
      ->SetConfiguration(brave::mojom::DynamicParams::New(
          brave_use_native_ethereum_wallet,
          allow_overwrite_window_ethereum_provider,
          brave_use_native_solana_wallet,
          allow_overwrite_window_solana_provider, de_amp_enabled));
}

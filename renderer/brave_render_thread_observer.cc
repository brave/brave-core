/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_render_thread_observer.h"

#include <utility>

#include "base/no_destructor.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace {

brave::mojom::DynamicParams* GetDynamicConfigParams() {
  static base::NoDestructor<brave::mojom::DynamicParams> dynamic_params;
  return dynamic_params.get();
}

}  // namespace

BraveRenderThreadObserver::BraveRenderThreadObserver() = default;

BraveRenderThreadObserver::~BraveRenderThreadObserver() = default;

// static
const brave::mojom::DynamicParams&
BraveRenderThreadObserver::GetDynamicParams() {
  return *GetDynamicConfigParams();
}

void BraveRenderThreadObserver::RegisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  associated_interfaces->AddInterface<brave::mojom::BraveRendererConfiguration>(
      base::BindRepeating(
          &BraveRenderThreadObserver::OnRendererConfigurationAssociatedRequest,
          base::Unretained(this)));
}

void BraveRenderThreadObserver::UnregisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  associated_interfaces->RemoveInterface(
      brave::mojom::BraveRendererConfiguration::Name_);
}

void BraveRenderThreadObserver::OnRendererConfigurationAssociatedRequest(
    mojo::PendingAssociatedReceiver<brave::mojom::BraveRendererConfiguration>
        receiver) {
  renderer_configuration_receivers_.Add(this, std::move(receiver));
}

void BraveRenderThreadObserver::SetInitialConfiguration(
    brave::mojom::InitialParamsPtr params) {
  initial_params_ = std::move(params);
}

void BraveRenderThreadObserver::SetConfiguration(
    brave::mojom::DynamicParamsPtr params) {
  *GetDynamicConfigParams() = std::move(*params);
}

bool BraveRenderThreadObserver::IsOnionAllowed() const {
  return (initial_params_ && initial_params_->is_tor_process) ||
         !GetDynamicConfigParams()->onion_only_in_tor_windows;
}

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
bool BraveRenderThreadObserver::IsBraveWalletAvailable() const {
  return initial_params_ && initial_params_->is_brave_wallet_available;
}
#endif

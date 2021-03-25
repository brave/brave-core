/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_render_thread_observer.h"

#include <utility>

#include "base/logging.h"
#include "base/no_destructor.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace {

brave::mojom::DynamicParams* GetDynamicConfigParams() {
  static base::NoDestructor<brave::mojom::DynamicParams> dynamic_params;
  return dynamic_params.get();
}

}  // namespace

BraveRenderThreadObserver::BraveRenderThreadObserver() {}

BraveRenderThreadObserver::~BraveRenderThreadObserver() {}

// static
const brave::mojom::DynamicParams&
BraveRenderThreadObserver::GetDynamicParams() {
  return *GetDynamicConfigParams();
}

void BraveRenderThreadObserver::RegisterMojoInterfaces(
    blink::AssociatedInterfaceRegistry* associated_interfaces) {
  associated_interfaces->AddInterface(base::BindRepeating(
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

void BraveRenderThreadObserver::SetConfiguration(
    brave::mojom::DynamicParamsPtr params) {
  *GetDynamicConfigParams() = std::move(*params);
}

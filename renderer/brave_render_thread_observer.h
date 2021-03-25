/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_RENDER_THREAD_OBSERVER_H_
#define BRAVE_RENDERER_BRAVE_RENDER_THREAD_OBSERVER_H_

#include "brave/common/brave_renderer_configuration.mojom.h"
#include "content/public/renderer/render_thread_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace blink {
class AssociatedInterfaceRegistry;
}

class BraveRenderThreadObserver
    : public content::RenderThreadObserver,
      public brave::mojom::BraveRendererConfiguration {
 public:
  BraveRenderThreadObserver(const BraveRenderThreadObserver&) = delete;
  BraveRenderThreadObserver& operator=(const BraveRenderThreadObserver&) =
      delete;
  BraveRenderThreadObserver();
  ~BraveRenderThreadObserver() override;

  // Return the dynamic parameters - those that may change while the
  // render process is running.
  static const brave::mojom::DynamicParams& GetDynamicParams();

 private:
  // content::RenderThreadObserver:
  void RegisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) override;
  void UnregisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) override;

  // brave::mojom::BraveRendererConfiguration:
  void SetConfiguration(brave::mojom::DynamicParamsPtr params) override;

  void OnRendererConfigurationAssociatedRequest(
      mojo::PendingAssociatedReceiver<brave::mojom::BraveRendererConfiguration>
          receiver);

  mojo::AssociatedReceiverSet<brave::mojom::BraveRendererConfiguration>
      renderer_configuration_receivers_;
};

#endif  // BRAVE_RENDERER_BRAVE_RENDER_THREAD_OBSERVER_H_

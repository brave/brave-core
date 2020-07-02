/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_SERVICE_H_

#include <memory>
#include <string>

#include "brave/components/services/tor/public/interfaces/tor.mojom.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_binding.h"
#include "services/service_manager/public/cpp/service_keepalive.h"

namespace tor {

class TorLauncherImpl;

class TorLauncherService : public service_manager::Service {
 public:
  explicit TorLauncherService(
      service_manager::mojom::ServiceRequest request);
  ~TorLauncherService() override;

  // Lifescycle events that occur after the service has started to spinup.
  void OnStart() override;
  void OnConnect(const service_manager::ConnectSourceInfo& source_info,
                 const std::string& interface_name,
                 mojo::ScopedMessagePipeHandle receiver_pipe) override;

 private:
  service_manager::ServiceBinding service_binding_;
  service_manager::ServiceKeepalive service_keepalive_;
  mojo::BinderMap binders_;

  class LauncherContext {
   public:
    explicit LauncherContext(TorLauncherImpl* impl) : impl_(impl) {}
    TorLauncherImpl* impl() const { return impl_; }
   private:
    TorLauncherImpl* impl_;
  };
  mojo::UniqueReceiverSet<tor::mojom::TorLauncher, LauncherContext> receivers_;

  void BindTorLauncherReceiver(
      service_manager::ServiceKeepalive* keepalive,
      mojo::PendingReceiver<tor::mojom::TorLauncher> receiver);
  void OnRemoteDisconnected();

  DISALLOW_COPY_AND_ASSIGN(TorLauncherService);
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_SERVICES_TOR_TOR_LAUNCHER_SERVICE_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/tor/tor_launcher_service.h"

#include <utility>

#include "build/build_config.h"
#include "brave/components/services/tor/tor_launcher_impl.h"

namespace tor {

void TorLauncherService::BindTorLauncherReceiver(
    service_manager::ServiceKeepalive* keepalive,
    mojo::PendingReceiver<tor::mojom::TorLauncher> receiver) {
  receivers_.Add(
      std::make_unique<tor::TorLauncherImpl>(keepalive->CreateRef()),
      std::move(receiver));
}

TorLauncherService::TorLauncherService(
        service_manager::mojom::ServiceRequest request) :
    service_binding_(this, std::move(request)),
    service_keepalive_(&service_binding_, base::TimeDelta()) {
}

TorLauncherService::~TorLauncherService() {}

void TorLauncherService::OnStart() {
  binders_.Add(base::BindRepeating(
      &TorLauncherService::BindTorLauncherReceiver,
      base::Unretained(this),
      &service_keepalive_));
}

void TorLauncherService::OnConnect(
    const service_manager::ConnectSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle receiver_pipe) {
  binders_.TryBind(interface_name, &receiver_pipe);
}

}  // namespace tor

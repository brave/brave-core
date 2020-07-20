/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/tor/tor_launcher_service.h"

#include <utility>

#include "build/build_config.h"
#include "brave/components/services/tor/tor_launcher_impl.h"
#include "mojo/public/cpp/bindings/generic_pending_receiver.h"

namespace tor {

void TorLauncherService::BindTorLauncherReceiver(
    service_manager::ServiceKeepalive* keepalive,
    mojo::PendingReceiver<tor::mojom::TorLauncher> receiver) {
  std::unique_ptr<tor::TorLauncherImpl> launcher =
      std::make_unique<tor::TorLauncherImpl>(keepalive->CreateRef());
  LauncherContext ctx(launcher.get());
  receivers_.Add(std::move(launcher), std::move(receiver), ctx);
}

TorLauncherService::TorLauncherService(
        mojo::PendingReceiver<service_manager::mojom::Service> receiver) :
    service_receiver_(this, std::move(receiver)),
    service_keepalive_(&service_receiver_, base::TimeDelta()) {
  receivers_.set_disconnect_handler(base::BindRepeating(
      &TorLauncherService::OnRemoteDisconnected, base::Unretained(this)));
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
  auto receiver =
      mojo::GenericPendingReceiver(interface_name, std::move(receiver_pipe));
  ignore_result(binders_.TryBind(&receiver));
}

void TorLauncherService::OnRemoteDisconnected() {
  receivers_.current_context().impl()->SetDisconnected();
}

}  // namespace tor

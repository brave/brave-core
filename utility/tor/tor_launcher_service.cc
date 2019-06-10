/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/tor/tor_launcher_service.h"

#include <utility>

#include "brave/utility/tor/tor_launcher_impl.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace tor {

namespace {

void OnTorLauncherRequest(service_manager::ServiceKeepalive* keepalive,
                          tor::mojom::TorLauncherRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<tor::TorLauncherImpl>(keepalive->CreateRef()),
      std::move(request));
}

}  // namespace

TorLauncherService::TorLauncherService(
    service_manager::mojom::ServiceRequest request)
    : service_binding_(this, std::move(request)),
      service_keepalive_(&service_binding_, base::TimeDelta()) {}

TorLauncherService::~TorLauncherService() {}

void TorLauncherService::OnStart() {
  registry_.AddInterface(
      base::BindRepeating(&OnTorLauncherRequest, &service_keepalive_));
}

void TorLauncherService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace tor

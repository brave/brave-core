/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/tor/tor_launcher_service.h"

#include <utility>

#include "build/build_config.h"
#include "brave/utility/tor/tor_launcher_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

void OnTorLauncherRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    tor::mojom::TorLauncherRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<tor::TorLauncherImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

namespace tor {

TorLauncherService::TorLauncherService() {}

TorLauncherService::~TorLauncherService() {}

std::unique_ptr<service_manager::Service>
TorLauncherService::CreateService() {
  return std::make_unique<TorLauncherService>();
}

void TorLauncherService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  registry_.AddInterface(
      base::Bind(&OnTorLauncherRequest, ref_factory_.get()));
}

void TorLauncherService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace tor

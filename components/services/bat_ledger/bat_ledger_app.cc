/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_app.h"

#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace bat_ledger {

namespace {

void OnBatLedgerServiceRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    bat_ledger::mojom::BatLedgerServiceRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<bat_ledger::BatLedgerServiceImpl>(ref_factory->CreateRef()),
      std::move(request));
}

} // namespace

BatLedgerApp::BatLedgerApp() {}

BatLedgerApp::~BatLedgerApp() {}

// static
std::unique_ptr<service_manager::Service>
BatLedgerApp::CreateService() {
  return std::make_unique<BatLedgerApp>();
}

void BatLedgerApp::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  registry_.AddInterface(
      base::Bind(&OnBatLedgerServiceRequest, ref_factory_.get()));
}

void BatLedgerApp::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

} // namespace bat_ledger

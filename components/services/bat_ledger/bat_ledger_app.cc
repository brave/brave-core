/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_app.h"

#include <string>
#include <utility>

#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace bat_ledger {

namespace {

void OnBatLedgerServiceRequest(
#if $CHROMIUM_CR72 == 0
    service_manager::ServiceContextRefFactory* ref_factory,
#else
    service_manager::ServiceKeepalive* keepalive,
#endif
    bat_ledger::mojom::BatLedgerServiceRequest request) {

#if $CHROMIUM_CR72 == 0
  mojo::MakeStrongBinding(
      std::make_unique<bat_ledger::BatLedgerServiceImpl>(ref_factory->CreateRef()),
      std::move(request));
#else
  mojo::MakeStrongBinding(
      std::make_unique<bat_ledger::BatLedgerServiceImpl>(
          keepalive->CreateRef()), std::move(request));
#endif
}

}  // namespace

#if $CHROMIUM_CR72 == 0
  BatLedgerApp::BatLedgerApp() {}


  // static
  std::unique_ptr<service_manager::Service>
  BatLedgerApp::CreateService() {
    return std::make_unique<BatLedgerApp>();
  }
#else
  BatLedgerApp::BatLedgerApp(
        service_manager::mojom::ServiceRequest request) :
    service_binding_(this, std::move(request)),
    service_keepalive_(&service_binding_, base::TimeDelta()) {
  }
#endif

BatLedgerApp::~BatLedgerApp() {}

void BatLedgerApp::OnStart() {
#if $CHROMIUM_CR72 == 0
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  registry_.AddInterface(
      base::Bind(&OnBatLedgerServiceRequest, ref_factory_.get()));
#else
  registry_.AddInterface(
      base::BindRepeating(&OnBatLedgerServiceRequest, &service_keepalive_));
#endif
}

void BatLedgerApp::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace bat_ledger

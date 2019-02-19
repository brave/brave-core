/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_APP_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_APP_H_

#include "brave/browser/version_info.h"

#include <memory>
#include <string>

#include "services/service_manager/public/cpp/binder_registry.h"
#if $CHROMIUM_CR72 == 0
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#else
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_binding.h"
#include "services/service_manager/public/cpp/service_keepalive.h"
#endif

namespace bat_ledger {

class BatLedgerApp : public service_manager::Service {
 public:
  ~BatLedgerApp() override;

#if $CHROMIUM_CR72 == 0
  BatLedgerApp();
  // Factory method for creating the service.
  static std::unique_ptr<service_manager::Service> CreateService();
#else
  explicit BatLedgerApp(service_manager::mojom::ServiceRequest request);
#endif

  // Lifescycle events that occur after the service has started to spinup.
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
#if $CHROMIUM_CR72 == 0
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
#else
  service_manager::ServiceBinding service_binding_;
  service_manager::ServiceKeepalive service_keepalive_;
#endif
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(BatLedgerApp);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_APP_H_

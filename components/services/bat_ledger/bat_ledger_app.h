/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_APP_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_APP_H_

#include <memory>

#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace bat_ledger {

class BatLedgerApp : public service_manager::Service {
 public:
  BatLedgerApp();
  ~BatLedgerApp() override;

  // Factory method for creating the service.
  static std::unique_ptr<service_manager::Service> CreateService();

  // Lifescycle events that occur after the service has started to spinup.
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  // State needed to manage service lifecycle and lifecycle of bound clients.
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(BatLedgerApp);
};

} // namespace rewards

#endif // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_APP_H_

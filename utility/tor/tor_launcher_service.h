/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_TOR_TOR_LAUNCHER_SERVICE_H_
#define BRAVE_UTILITY_TOR_TOR_LAUNCHER_SERVICE_H_

#include <memory>
#include <string>

#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_binding.h"
#include "services/service_manager/public/cpp/service_keepalive.h"

namespace tor {

class TorLauncherService : public service_manager::Service {
 public:
  explicit TorLauncherService(service_manager::mojom::ServiceRequest request);
  ~TorLauncherService() override;

  // Lifescycle events that occur after the service has started to spinup.
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  service_manager::ServiceBinding service_binding_;
  service_manager::ServiceKeepalive service_keepalive_;
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(TorLauncherService);
};

}  // namespace tor

#endif  // BRAVE_UTILITY_TOR_TOR_LAUNCHER_SERVICE_H_

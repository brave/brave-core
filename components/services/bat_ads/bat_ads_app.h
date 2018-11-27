/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_APP_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_APP_H_

#include <memory>

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace bat_ads {

class BatAdsApp : public service_manager::Service {
 public:
  static std::unique_ptr<service_manager::Service> CreateService();

  BatAdsApp();
  ~BatAdsApp() override;

 private:
  // |Service| override:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::BatAdsService> bindings_;

  DISALLOW_COPY_AND_ASSIGN(BatAdsApp);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_APP_H_

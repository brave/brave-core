/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_app.h"

#include "bat/ads/ads.h"
#include "brave/components/services/bat_ads/bat_ads_service_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace bat_ads {

namespace {

void OnBatAdsCreatorRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    bat_ads::mojom::BatAdsServiceRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<BatAdsServiceImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

// static
std::unique_ptr<service_manager::Service>
BatAdsApp::CreateService() {
  return std::make_unique<BatAdsApp>();
}

BatAdsApp::BatAdsApp() {}

BatAdsApp::~BatAdsApp() {}

void BatAdsApp::OnStart() {
#if defined(OFFICIAL_BUILD)
  ads::_is_production = true;
#else
  ads::_is_production = false;
#endif

#if defined(NDEBUG)
  ads::_is_debug = false;
#else
  ads::_is_debug = true;
#endif
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::Bind(&OnBatAdsCreatorRequest, ref_factory_.get()));
}

void BatAdsApp::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace bat_ads

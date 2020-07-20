/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_app.h"

#include <string>
#include <utility>

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"
#include "mojo/public/cpp/bindings/generic_pending_receiver.h"

namespace bat_ads {

void BatAdsApp::BindBatAdsServiceReceiver(
    service_manager::ServiceKeepalive* keepalive,
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService> receiver) {
  receivers_.Add(
      std::make_unique<bat_ads::BatAdsServiceImpl>(keepalive->CreateRef()),
      std::move(receiver));
}

BatAdsApp::BatAdsApp(
        mojo::PendingReceiver<service_manager::mojom::Service> receiver) :
    service_receiver_(this, std::move(receiver)),
    service_keepalive_(&service_receiver_, base::TimeDelta()) {
}

BatAdsApp::~BatAdsApp() {}

void BatAdsApp::OnStart() {
  binders_.Add(base::BindRepeating(&BatAdsApp::BindBatAdsServiceReceiver,
                                   base::Unretained(this),
                                   &service_keepalive_));
}

void BatAdsApp::OnConnect(
    const service_manager::ConnectSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle receiver_pipe) {
  auto receiver =
      mojo::GenericPendingReceiver(interface_name, std::move(receiver_pipe));
  ignore_result(binders_.TryBind(&receiver));
}

}  // namespace bat_ads

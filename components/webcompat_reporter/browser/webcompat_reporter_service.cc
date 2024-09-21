// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "components/component_updater/component_updater_service.h"
namespace webcompat_reporter {

WebcompatReporterService::WebcompatReporterService(component_updater::ComponentUpdateService* component_update_service)
  : component_update_service_(component_update_service) {}

WebcompatReporterService::~WebcompatReporterService() = default;

mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterService::MakeRemote() {
  mojo::PendingRemote<mojom::WebcompatReporterHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void WebcompatReporterService::Bind(
    mojo::PendingReceiver<mojom::WebcompatReporterHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void WebcompatReporterService::GetAdblockComponentInfo(
    GetAdblockComponentInfoCallback callback) {
  DCHECK(component_update_service_);
  LOG(INFO)
      << "[WEBCOMPAT] WebcompatReporterService::GetAdblockComponentInfo #100";
  auto components(component_update_service_->GetComponents());
  std::vector<mojom::AdblockComponentInfoPtr> result;
  for (auto& ci : components) {
    result.push_back(mojom::AdblockComponentInfo::New(
        base::UTF16ToUTF8(ci.name), ci.version.GetString(), ci.id));
  }
  std::move(callback).Run(std::move(result));
}

}  // namespace webcompat_reporter

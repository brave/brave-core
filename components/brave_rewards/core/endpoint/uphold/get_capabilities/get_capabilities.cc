/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"

#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

using uphold::Capabilities;

namespace endpoint {
namespace uphold {

GetCapabilities::GetCapabilities(RewardsEngineImpl& engine) : engine_(engine) {}

GetCapabilities::~GetCapabilities() = default;

void GetCapabilities::Request(const std::string& token,
                              GetCapabilitiesCallback callback) const {
  auto request = mojom::UrlRequest::New();

  request->url = engine_->Get<EnvironmentConfig>()
                     .uphold_api_url()
                     .Resolve("/v0/me/capabilities")
                     .spec();

  request->headers = {"Authorization: Bearer " + token};

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&GetCapabilities::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void GetCapabilities::OnRequest(GetCapabilitiesCallback callback,
                                mojom::UrlResponsePtr response) const {
  DCHECK(response);

  auto [result, capability_map] = ProcessResponse(*response);

  Capabilities capabilities;
  if (capability_map.count("receives") && capability_map.count("sends")) {
    const auto receives = capability_map["receives"];
    const auto sends = capability_map["sends"];

    capabilities.can_receive = receives.enabled && receives.requirements_empty;
    capabilities.can_send = sends.enabled && sends.requirements_empty;
  }

  std::move(callback).Run(result, std::move(capabilities));
}

std::pair<mojom::Result, GetCapabilities::CapabilityMap>
GetCapabilities::ProcessResponse(const mojom::UrlResponse& response) const {
  const auto status_code = response.status_code;

  if (status_code == net::HTTP_UNAUTHORIZED) {
    engine_->Log(FROM_HERE)
        << "Unauthorized access, HTTP status: " << status_code;
    return {mojom::Result::EXPIRED_TOKEN, {}};
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return {mojom::Result::FAILED, {}};
  }

  auto capability_map = ParseBody(response.body);
  return {!capability_map.empty() ? mojom::Result::OK : mojom::Result::FAILED,
          std::move(capability_map)};
}

GetCapabilities::CapabilityMap GetCapabilities::ParseBody(
    const std::string& body) const {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    engine_->LogError(FROM_HERE) << "Invalid body format";
    return {};
  }

  std::map<std::string, Capability> capability_map;
  for (const auto& item : value->GetList()) {
    DCHECK(item.is_dict());
    const auto& dict = item.GetDict();
    const auto* key = dict.FindString("key");
    const auto enabled = dict.FindBool("enabled");
    const auto* requirements = dict.FindList("requirements");

    if (!key || !enabled || !requirements) {
      capability_map.clear();
      break;
    }

    capability_map.emplace(*key, Capability{*enabled, requirements->empty()});
  }

  if (capability_map.empty()) {
    engine_->LogError(FROM_HERE) << "Invalid body format";
  }

  return capability_map;
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal

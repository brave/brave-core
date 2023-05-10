/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"

#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {

using uphold::Capabilities;

namespace endpoint {
namespace uphold {

GetCapabilities::GetCapabilities(LedgerImpl& ledger) : ledger_(ledger) {}

GetCapabilities::~GetCapabilities() = default;

void GetCapabilities::Request(const std::string& token,
                              GetCapabilitiesCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetServerUrl("/v0/me/capabilities");
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request),
                   base::BindOnce(&GetCapabilities::OnRequest,
                                  base::Unretained(this), std::move(callback)));
}

void GetCapabilities::OnRequest(GetCapabilitiesCallback callback,
                                mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

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
GetCapabilities::ProcessResponse(const mojom::UrlResponse& response) {
  const auto status_code = response.status_code;

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(1, "Unauthorized access, HTTP status: " << status_code);
    return {mojom::Result::EXPIRED_TOKEN, {}};
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return {mojom::Result::LEDGER_ERROR, {}};
  }

  auto capability_map = ParseBody(response.body);
  return {!capability_map.empty() ? mojom::Result::LEDGER_OK
                                  : mojom::Result::LEDGER_ERROR,
          std::move(capability_map)};
}

GetCapabilities::CapabilityMap GetCapabilities::ParseBody(
    const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    BLOG(0, "Invalid body format!");
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
    BLOG(0, "Invalid body format!");
  }

  return capability_map;
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/get_capabilities/get_capabilities.h"

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using ledger::uphold::Capabilities;

namespace ledger {
namespace endpoint {
namespace uphold {

GetCapabilities::GetCapabilities(LedgerImpl* ledger)
    : ledger_((DCHECK(ledger), ledger)) {}

GetCapabilities::~GetCapabilities() = default;

void GetCapabilities::Request(const std::string& token,
                              GetCapabilitiesCallback callback) {
  auto request = type::UrlRequest::New();
  request->url = GetServerUrl("/v0/me/capabilities");
  request->headers = RequestAuthorization(token);
  ledger_->LoadURL(std::move(request),
                   base::BindOnce(&GetCapabilities::OnRequest,
                                  base::Unretained(this), std::move(callback)));
}

void GetCapabilities::OnRequest(GetCapabilitiesCallback callback,
                                const type::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);

  auto [result, capability_map] = ProcessResponse(response);

  Capabilities capabilities;
  if (capability_map.count("receives") && capability_map.count("sends")) {
    const auto receives = capability_map["receives"];
    const auto sends = capability_map["sends"];

    capabilities.can_receive = receives.enabled && receives.requirements_empty;
    capabilities.can_send = sends.enabled && sends.requirements_empty;
  }

  std::move(callback).Run(result, std::move(capabilities));
}

std::pair<type::Result, GetCapabilities::CapabilityMap>
GetCapabilities::ProcessResponse(const type::UrlResponse& response) {
  const auto status_code = response.status_code;

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(1, "Unauthorized access, HTTP status: " << status_code);
    return {type::Result::EXPIRED_TOKEN, {}};
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return {type::Result::LEDGER_ERROR, {}};
  }

  auto capability_map = ParseBody(response.body);
  return {!capability_map.empty() ? type::Result::LEDGER_OK
                                  : type::Result::LEDGER_ERROR,
          std::move(capability_map)};
}

GetCapabilities::CapabilityMap GetCapabilities::ParseBody(
    const std::string& body) {
  std::map<std::string, Capability> capability_map;

  const auto value = base::JSONReader::Read(body);
  const base::ListValue* list_value = nullptr;

  if (value && value->GetAsList(&list_value)) {
    DCHECK(list_value);

    for (const auto& item : list_value->GetList()) {
      const auto* key = item.FindStringKey("key");
      const auto enabled = item.FindBoolKey("enabled");
      const auto* requirements = item.FindListKey("requirements");

      if (!key || !enabled || !requirements) {
        capability_map.clear();
        break;
      }

      capability_map.emplace(
          *key, Capability{*enabled, requirements->GetList().empty()});
    }
  }

  if (capability_map.empty()) {
    BLOG(0, "Invalid body format!");
  }

  return capability_map;
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger

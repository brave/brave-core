/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/car_content_requester.h"
#include <string>

#include "base/memory/scoped_refptr.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

constexpr char kGatewayUrlFormatParamName[] = "format";
constexpr char kGatewayUrlFormatParamVal[] = "car";

constexpr char kGatewayUrlDagScopeParamName[] = "dag-scope";
constexpr char kGatewayUrlDagScopeParamVal[] = "entity";

constexpr char kGatewayUrlEntityBytesParamName[] = "entity-bytes";
constexpr char kGatewayUrlEntityBytesOnlyStructParamVal[] = "0:0";

constexpr char kGatewayUrlBlockOrderParamName[] = "order";
constexpr char kGatewayUrlBlockOrderParamVal[] = "dfs";

void ApplyRequestParameters(GURL& car_request_url) {
  car_request_url = net::AppendQueryParameter(
      car_request_url, kGatewayUrlFormatParamName, kGatewayUrlFormatParamVal);
  car_request_url =
      net::AppendQueryParameter(car_request_url, kGatewayUrlDagScopeParamName,
                                kGatewayUrlDagScopeParamVal);
  car_request_url =
      net::AppendQueryParameter(car_request_url, kGatewayUrlBlockOrderParamName,
                                kGatewayUrlBlockOrderParamVal);
}

}  // namespace

namespace ipfs::ipld {

CarContentRequester::CarContentRequester(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs,
    const bool only_metadata)
    : ContentRequester(url, url_loader_factory, prefs),
      only_metadata_(only_metadata) {}

CarContentRequester::~CarContentRequester() = default;

GURL CarContentRequester::GetGatewayRequestUrl() const {
  auto car_request_url = ContentRequester::GetGatewayRequestUrl();
  ApplyRequestParameters(car_request_url);

  if (only_metadata_) {
    car_request_url = net::AppendQueryParameter(
        car_request_url, kGatewayUrlEntityBytesParamName,
        kGatewayUrlEntityBytesOnlyStructParamVal);
  }
LOG(INFO) << "[IPFS] GetGatewayRequestUrl() car_request_url:" << car_request_url;
  return car_request_url;
}

std::unique_ptr<network::SimpleURLLoader> CarContentRequester::CreateLoader()
    const {
  return CreateURLLoader(GetGatewayRequestUrl(), "GET");
}

}  // namespace ipfs::ipld

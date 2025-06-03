/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "brave/components/brave_account/endpoints/verify/init.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace brave_account::endpoints {
namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation(
      "brave_account_endpoints_verify_init", R"(
    semantics {
      sender: "Brave Account client"
      description: "..."
      trigger: "..."
      data: "..."
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      policy_exception_justification: "Not implemented."
    }
  )");
}

}  // namespace

VerifyInit::VerifyInit(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

void VerifyInit::Send(
    const std::string& email,
    api_request_helper::APIRequestHelper::ResultCallback callback) {
  base::Value::Dict dict;
  dict.Set("service", "accounts");
  dict.Set("intent", "registration");
  dict.Set("email", email);

  std::string json;
  base::JSONWriter::Write(dict, &json);

  api_request_helper_.Request(
      "POST", GURL("https://accounts.bsg.bravesoftware.com/v2/verify/init"),
      json, "application/json", std::move(callback));
}

}  // namespace brave_account::endpoints

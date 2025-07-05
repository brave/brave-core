/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password/finalize.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace brave_account::endpoints {
namespace {

constexpr char kPasswordFinalizeHostnamePart[] = "accounts.bsg";
constexpr char kPasswordFinalizePath[] = "v2/accounts/password/finalize";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation(
      "brave_account_endpoints_password_finalize", R"(
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

PasswordFinalize::PasswordFinalize(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

PasswordFinalize::~PasswordFinalize() = default;

void PasswordFinalize::Send(const std::string& verification_token,
                            const std::string& serialized_record,
                            base::OnceCallback<void(bool)> callback) {
  base::Value::Dict dict;
  dict.Set("serializedRecord", serialized_record);

  std::string json;
  base::JSONWriter::Write(dict, &json);

  base::flat_map<std::string, std::string> headers;
  headers.emplace("Authorization",
                  base::StrCat({"Bearer ", verification_token}));

  auto endpoint_url =
      GURL(base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                         brave_domains::GetServicesDomain(
                             kPasswordFinalizeHostnamePart)}))
          .Resolve(kPasswordFinalizePath);
  api_request_helper_.Request(
      "POST", endpoint_url, json, "application/json",
      base::BindOnce(&PasswordFinalize::OnResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      headers);
}

void PasswordFinalize::OnResponse(base::OnceCallback<void(bool)> callback,
                                  api_request_helper::APIRequestResult result) {
  DVLOG(0) << result.response_code();
  DVLOG(0) << result.value_body();
  std::move(callback).Run(result.Is2XXResponseCode());
}

}  // namespace brave_account::endpoints

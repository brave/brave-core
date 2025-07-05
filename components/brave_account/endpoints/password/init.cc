/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password/init.h"

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

constexpr char kPasswordInitHostnamePart[] = "accounts.bsg";
constexpr char kPasswordInitPath[] = "v2/accounts/password/init";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation(
      "brave_account_endpoints_password_init", R"(
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

PasswordInit::PasswordInit(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

PasswordInit::~PasswordInit() = default;

void PasswordInit::Send(const std::string& email,
                        const std::string& blinded_message,
                        base::OnceCallback<void(const std::string&)> callback) {
  base::Value::Dict dict;
  dict.Set("blindedMessage", blinded_message);
  dict.Set("newAccountEmail", email);
  dict.Set("serializeResponse", true);

  std::string json;
  base::JSONWriter::Write(dict, &json);

  auto endpoint_url =
      GURL(base::StrCat(
               {url::kHttpsScheme, url::kStandardSchemeSeparator,
                brave_domains::GetServicesDomain(kPasswordInitHostnamePart)}))
          .Resolve(kPasswordInitPath);
  api_request_helper_.Request(
      "POST", endpoint_url, json, "application/json",
      base::BindOnce(&PasswordInit::OnResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void PasswordInit::OnResponse(
    base::OnceCallback<void(const std::string&)> callback,
    api_request_helper::APIRequestResult result) {
  DVLOG(0) << result.response_code();
  DVLOG(0) << result.value_body();
  if (result.Is2XXResponseCode() && result.value_body().is_dict()) {
    const std::string* serialized_response =
        result.value_body().GetDict().FindString("serializedResponse");
    const std::string* verification_token =
        result.value_body().GetDict().FindString("verificationToken");

    // Create a combined response with both serialized response and verification
    // token
    base::Value::Dict response_dict;
    if (serialized_response) {
      response_dict.Set("serializedResponse", *serialized_response);
    }
    if (verification_token) {
      response_dict.Set("verificationToken", *verification_token);
    }

    std::string combined_response;
    base::JSONWriter::Write(response_dict, &combined_response);
    std::move(callback).Run(combined_response);
  } else {
    std::move(callback).Run("");
  }
}

}  // namespace brave_account::endpoints

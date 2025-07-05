/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password/init.h"

#include <utility>

#include "base/json/json_writer.h"
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
      description: "Initiates the creation process for a Brave Account."
      trigger: "User attempts to create a new Brave Account from settings."
      user_data: {
        type: EMAIL
      }
      data:
        "Blinded cryptographic message for secure password setup "
        "and account email address."
      destination: OTHER
      destination_other: "Brave Account service"
    }
    policy {
      cookies_allowed: NO
      policy_exception_justification:
        "This request is essential for creating a Brave Account and "
        "cannot be disabled by policy."
    }
  )");
}

}  // namespace

PasswordInit::PasswordInit(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

PasswordInit::~PasswordInit() = default;

void PasswordInit::Send(
    const std::string& email,
    const std::string& blinded_message,
    api_request_helper::APIRequestHelper::ResultCallback callback) {
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
  api_request_helper_.Request("POST", endpoint_url, json, "application/json",
                              std::move(callback));
}

}  // namespace brave_account::endpoints

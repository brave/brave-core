/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

namespace {
constexpr char kVpnHost[] = "housekeeping.sudosecuritygroup.com";

constexpr char kAllServerRegions[] = "api/v1/servers/all-server-regions";
constexpr char kTimezonesForRegions[] =
    "api/v1.1/servers/timezones-for-regions";
constexpr char kHostnameForRegion[] = "api/v1/servers/hostnames-for-region";
constexpr char kCreateSubscriberCredential[] =
    "api/v1/subscriber-credential/create";
constexpr char kVerifyPurchaseToken[] = "api/v1.1/verify-purchase-token";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_vpn_service", R"(
      semantics {
        sender: "Brave VPN Service"
        description:
          "This service is used to communicate with Guardian VPN apis"
          "on behalf of the user interacting with the Brave VPN."
        trigger:
          "Triggered by user connecting the Brave VPN."
        data:
          "Servers, hosts and credentials for Brave VPN"
        destination: WEBSITE
      }
    )");
}

GURL GetURLWithPath(const std::string& host, const std::string& path) {
  return GURL(std::string(url::kHttpsScheme) + "://" + host).Resolve(path);
}

std::string CreateJSONRequestBody(const base::Value& dict) {
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

std::string GetSubscriberCredentialFromJson(const std::string& json) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return "";
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return "";
  }

  const base::Value* subscriber_credential =
      records_v->FindKey("subscriber-credential");
  return subscriber_credential == nullptr ? ""
                                          : subscriber_credential->GetString();
}

}  // namespace

BraveVpnService::BraveVpnService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

BraveVpnService::~BraveVpnService() = default;

void BraveVpnService::OAuthRequest(const GURL& url,
                                   const std::string& method,
                                   const std::string& post_data,
                                   bool set_app_ident,
                                   URLRequestCallback callback) {
  std::map<std::string, std::string> headers;
  if (set_app_ident) {
    headers["GRD-App-Ident"] = "Brave-Client";
  }

  api_request_helper_.Request(method, url, post_data, "application/json", false,
                              std::move(callback), headers);
}

void BraveVpnService::GetAllServerRegions(ResponseCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse, base::Unretained(this),
                     std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kAllServerRegions);
  OAuthRequest(base_url, "GET", "", false, std::move(internal_callback));
}

void BraveVpnService::GetTimezonesForRegions(ResponseCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse, base::Unretained(this),
                     std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kTimezonesForRegions);
  OAuthRequest(base_url, "GET", "", false, std::move(internal_callback));
}

void BraveVpnService::GetHostnamesForRegion(ResponseCallback callback,
                                            const std::string& region) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse, base::Unretained(this),
                     std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kHostnameForRegion);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("region", region);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, false,
               std::move(internal_callback));
}

void BraveVpnService::VerifyPurchaseToken(ResponseCallback callback,
                                          const std::string& purchase_token,
                                          const std::string& product_id,
                                          const std::string& product_type) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse, base::Unretained(this),
                     std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kVerifyPurchaseToken);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("purchase-token", purchase_token);
  dict.SetStringKey("product-id", product_id);
  dict.SetStringKey("product-type", product_type);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, true,
               std::move(internal_callback));
}

void BraveVpnService::OnGetResponse(
    ResponseCallback callback,
    int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string json_response;
  bool success = status == 200;
  if (success) {
    json_response = body;
  }
  std::move(callback).Run(json_response, success);
}

void BraveVpnService::GetSubscriberCredential(
    ResponseCallback callback,
    const std::string& product_type,
    const std::string& product_id,
    const std::string& validation_method,
    const std::string& purchase_token) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetSubscriberCredential,
                     base::Unretained(this), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kCreateSubscriberCredential);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("product-type", product_type);
  dict.SetStringKey("product-id", product_id);
  dict.SetStringKey("validation-method", validation_method);
  dict.SetStringKey("purchase-token", purchase_token);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, true,
               std::move(internal_callback));
}

void BraveVpnService::OnGetSubscriberCredential(
    ResponseCallback callback,
    int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string subscriber_credential;

  bool success = status == 200;
  if (success) {
    subscriber_credential = GetSubscriberCredentialFromJson(body);
  }
  std::move(callback).Run(subscriber_credential, success);
}

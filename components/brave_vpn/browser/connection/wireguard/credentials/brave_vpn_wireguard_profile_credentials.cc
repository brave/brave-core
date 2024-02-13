/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/credentials/brave_vpn_wireguard_profile_credentials.h"

#include <optional>
#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

namespace brave_vpn {

namespace wireguard {

WireguardProfileCredentials::WireguardProfileCredentials(
    const std::string& server_public_key,
    const std::string& client_private_key,
    const std::string& mapped_ip4_address,
    const std::string& client_id,
    const std::string& api_auth_token)
    : server_public_key(server_public_key),
      client_private_key(client_private_key),
      mapped_ip4_address(mapped_ip4_address),
      client_id(client_id),
      api_auth_token(api_auth_token) {}

WireguardProfileCredentials::WireguardProfileCredentials(
    const WireguardProfileCredentials& creds) {
  server_public_key = creds.server_public_key;
  client_private_key = creds.client_private_key;
  mapped_ip4_address = creds.mapped_ip4_address;
  client_id = creds.client_id;
  api_auth_token = creds.api_auth_token;
}

WireguardProfileCredentials::~WireguardProfileCredentials() = default;

std::optional<WireguardProfileCredentials>
WireguardProfileCredentials::FromServerResponse(
    const std::string& server_response,
    const std::string& client_private_key) {
  if (server_response.empty() || client_private_key.empty()) {
    return std::nullopt;
  }
  std::optional<base::Value> value = base::JSONReader::Read(server_response);
  if (!value.has_value()) {
    return std::nullopt;
  }
  auto* server_public_key =
      value->GetDict().FindStringByDottedPath("server-public-key");
  if (!server_public_key) {
    return std::nullopt;
  }
  auto* mapped_ip4_address =
      value->GetDict().FindStringByDottedPath("mapped-ipv4-address");
  if (!mapped_ip4_address) {
    return std::nullopt;
  }
  auto* client_id = value->GetDict().FindStringByDottedPath("client-id");
  if (!client_id) {
    return std::nullopt;
  }
  auto* api_auth_token =
      value->GetDict().FindStringByDottedPath("api-auth-token");
  if (!api_auth_token) {
    return std::nullopt;
  }
  return WireguardProfileCredentials(*server_public_key, client_private_key,
                                     *mapped_ip4_address, *client_id,
                                     *api_auth_token);
}

std::optional<WireguardProfileCredentials>
WireguardProfileCredentials::FromString(const std::string& credentials) {
  if (credentials.empty()) {
    return std::nullopt;
  }
  std::optional<base::Value> value = base::JSONReader::Read(credentials);
  if (!value.has_value()) {
    return std::nullopt;
  }
  auto* server_public_key =
      value->GetDict().FindStringByDottedPath("server-public-key");
  if (!server_public_key) {
    return std::nullopt;
  }
  auto* client_private_key =
      value->GetDict().FindStringByDottedPath("client-private-key");
  if (!client_private_key) {
    return std::nullopt;
  }
  auto* mapped_ip4_address =
      value->GetDict().FindStringByDottedPath("mapped-ipv4-address");
  if (!mapped_ip4_address) {
    return std::nullopt;
  }
  auto* client_id = value->GetDict().FindStringByDottedPath("client-id");
  if (!client_id) {
    return std::nullopt;
  }
  auto* api_auth_token =
      value->GetDict().FindStringByDottedPath("api-auth-token");
  if (!api_auth_token) {
    return std::nullopt;
  }
  return WireguardProfileCredentials(*server_public_key, *client_private_key,
                                     *mapped_ip4_address, *client_id,
                                     *api_auth_token);
}

bool WireguardProfileCredentials::IsValid() const {
  return !server_public_key.empty() && !client_private_key.empty() &&
         !mapped_ip4_address.empty() && !client_id.empty() &&
         !api_auth_token.empty();
}

std::optional<std::string> WireguardProfileCredentials::ToString() const {
  if (!IsValid()) {
    return std::nullopt;
  }

  base::Value::Dict data;
  data.Set("server-public-key", server_public_key);
  data.Set("client-private-key", client_private_key);
  data.Set("mapped-ipv4-address", mapped_ip4_address);
  data.Set("client-id", client_id);
  data.Set("api-auth-token", api_auth_token);
  std::string json_string;
  base::JSONWriter::Write(data, &json_string);
  return json_string;
}

}  // namespace wireguard

}  // namespace brave_vpn

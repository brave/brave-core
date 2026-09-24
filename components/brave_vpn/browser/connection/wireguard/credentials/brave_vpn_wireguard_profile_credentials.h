/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_CREDENTIALS_BRAVE_VPN_WIREGUARD_PROFILE_CREDENTIALS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_CREDENTIALS_BRAVE_VPN_WIREGUARD_PROFILE_CREDENTIALS_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"

namespace brave_vpn {

namespace wireguard {

struct WireguardProfileCredentials {
  WireguardProfileCredentials(const std::string& server_public_key,
                              const std::string& mapped_ip4_address,
                              const std::string& client_id,
                              const std::string& api_auth_token,
                              const std::string& client_private_key);
  ~WireguardProfileCredentials();

  WireguardProfileCredentials(const WireguardProfileCredentials& creds);
  bool operator==(const WireguardProfileCredentials& other) const {
    return server_public_key == other.server_public_key &&
           client_private_key == other.client_private_key &&
           mapped_ip4_address == other.mapped_ip4_address &&
           client_id == other.client_id &&
           api_auth_token == other.api_auth_token;
  }

  static std::optional<WireguardProfileCredentials> FromServerResponse(
      const std::string& credentials,
      const std::string& client_private_key);
  static std::optional<WireguardProfileCredentials> FromString(
      const std::string& credentials);

  std::optional<std::string> ToString() const;
  bool IsValid() const;

  std::string server_public_key;
  std::string client_private_key;

  std::string mapped_ip4_address;
  std::string client_id;
  std::string api_auth_token;
};

}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIREGUARD_CREDENTIALS_BRAVE_VPN_WIREGUARD_PROFILE_CREDENTIALS_H_

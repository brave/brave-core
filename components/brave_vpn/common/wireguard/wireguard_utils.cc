/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

#include <stdint.h>

#include <optional>
#include <vector>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "crypto/openssl_util.h"
#include "net/base/ip_address.h"
#include "net/base/url_util.h"
#include "third_party/boringssl/src/include/openssl/base64.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/url_util.h"

namespace brave_vpn {

namespace wireguard {

namespace {
std::string EncodeBase64(const std::vector<uint8_t>& in) {
  std::string res;
  size_t size = 0;
  if (!EVP_EncodedLength(&size, in.size())) {
    DCHECK(false);
    return "";
  }
  std::vector<uint8_t> out(size);
  size_t numEncBytes = EVP_EncodeBlock(&out.front(), &in.front(), in.size());
  DCHECK_NE(numEncBytes, 0u);
  res = reinterpret_cast<char*>(&out.front());
  return res;
}

constexpr char kCloudflareIPv4[] = "1.1.1.1";
// Template for wireguard config generation.
constexpr char kWireguardConfigTemplate[] = R"(
  [Interface]
  PrivateKey = {client_private_key}
  Address = {mapped_ipv4_address}
  DNS = {dns_servers}
  [Peer]
  PublicKey = {server_public_key}
  AllowedIPs = 0.0.0.0/0, ::/0
  Endpoint = {vpn_server_hostname}:51821
)";

}  // namespace

std::optional<std::string> CreateWireguardConfig(
    const std::string& client_private_key,
    const std::string& server_public_key,
    const std::string& vpn_server_hostname,
    const std::string& mapped_ipv4_address) {
  if (client_private_key.empty() || server_public_key.empty() ||
      vpn_server_hostname.empty() || mapped_ipv4_address.empty()) {
    return std::nullopt;
  }
  std::string config = kWireguardConfigTemplate;
  base::ReplaceSubstringsAfterOffset(&config, 0, "{client_private_key}",
                                     client_private_key);
  base::ReplaceSubstringsAfterOffset(&config, 0, "{server_public_key}",
                                     server_public_key);
  base::ReplaceSubstringsAfterOffset(&config, 0, "{vpn_server_hostname}",
                                     vpn_server_hostname);
  base::ReplaceSubstringsAfterOffset(&config, 0, "{mapped_ipv4_address}",
                                     mapped_ipv4_address);
  base::ReplaceSubstringsAfterOffset(&config, 0, "{dns_servers}",
                                     kCloudflareIPv4);
  return config;
}

WireguardKeyPair GenerateNewX25519Keypair() {
  uint8_t pubkey[32] = {}, privkey[32] = {};
  X25519_keypair(pubkey, privkey);
  return std::make_tuple(
      EncodeBase64(std::vector<uint8_t>(pubkey, pubkey + 32)),
      EncodeBase64(std::vector<uint8_t>(privkey, privkey + 32)));
}

std::optional<std::string> ValidateKey(const std::string& key,
                                       const std::string& field_name) {
  if (key.length() == 0) {
    VLOG(1) << "`" << field_name << "` does not have a value";
    return std::nullopt;
  }

  if (!re2::RE2::FullMatch(key, R"(^[-A-Za-z0-9+\/=]+$)")) {
    VLOG(1) << "`" << field_name << "` contains invalid characters";
    return std::nullopt;
  }

  std::string decoded_config;
  if (!base::Base64Decode(key, &decoded_config) || decoded_config.empty()) {
    VLOG(1) << "`" << field_name << "` is not base64 encoded";
    return std::nullopt;
  }

  if (decoded_config.length() != 32) {
    VLOG(1) << "`" << field_name << "` is not the correct length";
    return std::nullopt;
  }

  return key;
}

std::optional<std::string> ValidateAddress(const std::string& address) {
  if (!re2::RE2::FullMatch(address, R"(^[A-Za-z0-9._\-:[\]]+$)")) {
    VLOG(1) << "address contains invalid characters";
    return std::nullopt;
  }

  auto parsed = net::IPAddress::FromIPLiteral(address);
  if (!parsed.has_value()) {
    VLOG(1) << "failed parsing address";
    return std::nullopt;
  }

  auto parsed_ip = parsed.value();
  if (!parsed_ip.IsValid()) {
    VLOG(1) << "address is not valid";
    return std::nullopt;
  }

  if (!parsed_ip.IsIPv4()) {
    VLOG(1) << "address must be IPv4";
    return std::nullopt;
  }

  if (parsed_ip.IsLinkLocal() && parsed_ip.IsLoopback()) {
    VLOG(1) << "address should not be local / loopback";
    return std::nullopt;
  }

  return parsed_ip.ToString();
}

std::optional<std::string> ValidateEndpoint(const std::string& endpoint) {
  if (!re2::RE2::FullMatch(endpoint, R"(^[A-Za-z0-9._\-:]+$)")) {
    VLOG(1) << "endpoint contains invalid characters";
    return std::nullopt;
  }

  std::string parsed_host;
  int parsed_port = 0;
  if (!net::ParseHostAndPort(endpoint, &parsed_host, &parsed_port)) {
    VLOG(1) << "failed parsing endpoint";
    return std::nullopt;
  }

  if (!url::DomainIs(parsed_host, "guardianapp.com") &&
      !url::DomainIs(parsed_host, "sudosecuritygroup.com")) {
    VLOG(1) << "endpoint is not a valid hostname";
    return std::nullopt;
  }

  return parsed_host;
}

}  // namespace wireguard

}  // namespace brave_vpn

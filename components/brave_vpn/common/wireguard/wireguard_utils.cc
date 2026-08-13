/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

#include <stdint.h>

#include <optional>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "crypto/openssl_util.h"
#include "net/base/ip_address.h"
#include "net/base/url_util.h"
#include "third_party/boringssl/src/include/openssl/base64.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/url_util.h"

namespace brave_vpn {

namespace wireguard {

std::string EncodeBase64(base::span<const uint8_t> in) {
  size_t size = 0;
  CHECK(EVP_EncodedLength(&size, in.size()));
  std::vector<uint8_t> out(size);
  size_t bytes_encoded = EVP_EncodeBlock(&out.front(), &in.front(), in.size());
  return std::string(
      base::as_string_view(base::span(out).first(bytes_encoded)));
}

namespace {
constexpr char kCloudflareIPv4[] = "1.1.1.1";

// The whole address space minus private, link-local and multicast ranges
// (10/8, 172.16/12, 192.168/16, 169.254/16, 224/3, fc00::/7, fe80::/10,
// ff00::/8) so LAN devices and mDNS/SSDP discovery stay reachable while
// connected.
//
// On Windows this list must not contain any /0 prefix: tunnel.dll only installs
// its blockAll/blockDNS WFP filters when a peer routes a default route, and
// those filters are what block the LAN.
constexpr char kAllowedIPs[] =
    "0.0.0.0/5, 8.0.0.0/7, 11.0.0.0/8, 12.0.0.0/6, 16.0.0.0/4, 32.0.0.0/3, "
    "64.0.0.0/2, 128.0.0.0/3, 160.0.0.0/5, 168.0.0.0/8, 169.0.0.0/9, "
    "169.128.0.0/10, 169.192.0.0/11, 169.224.0.0/12, 169.240.0.0/13, "
    "169.248.0.0/14, 169.252.0.0/15, 169.255.0.0/16, 170.0.0.0/7, "
    "172.0.0.0/12, 172.32.0.0/11, 172.64.0.0/10, 172.128.0.0/9, 173.0.0.0/8, "
    "174.0.0.0/7, 176.0.0.0/4, 192.0.0.0/9, 192.128.0.0/11, 192.160.0.0/13, "
    "192.169.0.0/16, 192.170.0.0/15, 192.172.0.0/14, 192.176.0.0/12, "
    "192.192.0.0/10, 193.0.0.0/8, 194.0.0.0/7, 196.0.0.0/6, 200.0.0.0/5, "
    "208.0.0.0/4, ::/1, 8000::/2, c000::/3, e000::/4, f000::/5, f800::/6, "
    "fe00::/9, fec0::/10";

// Template for wireguard config generation.
// For a quick reference on the keys/values, please see:
// https://github.com/pirate/wireguard-docs?tab=readme-ov-file#config-reference
constexpr char kWireguardConfigTemplate[] = R"(
  [Interface]
  PrivateKey = {client_private_key}
  Address = {mapped_ipv4_address}
  DNS = {dns_servers}
  [Peer]
  PublicKey = {server_public_key}
  AllowedIPs = {allowed_ips}
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
  base::ReplaceSubstringsAfterOffset(&config, 0, "{allowed_ips}", kAllowedIPs);
  return config;
}

WireguardKeyPair GenerateNewX25519Keypair() {
  uint8_t pubkey[32] = {}, privkey[32] = {};
  X25519_keypair(pubkey, privkey);
  return std::make_tuple(
      EncodeBase64(std::vector<uint8_t>(pubkey, UNSAFE_TODO(pubkey + 32))),
      EncodeBase64(std::vector<uint8_t>(privkey, UNSAFE_TODO(privkey + 32))));
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

  if (parsed_ip.IsLinkLocal() || parsed_ip.IsLoopback()) {
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

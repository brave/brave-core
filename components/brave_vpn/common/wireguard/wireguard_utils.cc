/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/wireguard/wireguard_utils.h"

#include <stdint.h>

#include <optional>
#include <vector>

#include "base/base64.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "crypto/openssl_util.h"
#include "third_party/boringssl/src/include/openssl/base64.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"
#include "third_party/re2/src/re2/re2.h"

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

bool IsValidConfigDNS(const std::string& config_str) {
  int match_count = 0, any_match_count = 0;

  // Match our expected `DNS` field.
  absl::string_view input(config_str);
  while (match_count < 2 &&
         re2::RE2::FindAndConsume(&input, R"((?m)^\s*DNS = 1\.1\.1\.1$)")) {
    match_count++;
  }

  // Match any `DNS` field.
  absl::string_view input_any(config_str);
  while (any_match_count < 2 &&
         re2::RE2::FindAndConsume(&input_any, R"((?mi)^\s*DNS\s*=\s*.*$)")) {
    any_match_count++;
  }

  if (any_match_count == 1 && match_count == 1) {
    return true;
  }

  VLOG(1) << "Invalid configuration: " << any_match_count
          << " (or more) `DNS` fields found, " << match_count
          << " matching our expected `DNS` pattern.";
  return false;
}

bool IsValidConfigEndpoint(const std::string& config_str) {
  int match_count = 0, any_match_count = 0;

  // Match our expected `Endpoint` field.
  absl::string_view input(config_str);
  while (
      match_count < 2 &&
      re2::RE2::FindAndConsume(
          &input,
          R"((?m)^\s*Endpoint = [0-9A-z-_]{1,63}\.(guardianapp\.com|sudosecuritygroup\.com):51821$)")) {
    match_count++;
  }

  // Match any `Endpoint` field.
  absl::string_view input_any(config_str);
  while (
      any_match_count < 2 &&
      re2::RE2::FindAndConsume(&input_any, R"((?mi)^\s*Endpoint\s*=\s*.*$)")) {
    any_match_count++;
  }

  if (any_match_count == 1 && match_count == 1) {
    return true;
  }

  VLOG(1) << "Invalid configuration: " << any_match_count
          << " (or more) `Endpoint` fields found, " << match_count
          << " matching our expected `Endpoint` pattern.";
  return false;
}

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
  crypto::EnsureOpenSSLInit();
  uint8_t pubkey[32] = {}, privkey[32] = {};
  X25519_keypair(pubkey, privkey);
  return std::make_tuple(
      EncodeBase64(std::vector<uint8_t>(pubkey, pubkey + 32)),
      EncodeBase64(std::vector<uint8_t>(privkey, privkey + 32)));
}

// See https://github.com/google/re2/wiki/Syntax for RE2 syntax
bool IsValidConfig(const wchar_t* config) {
  if (!config) {
    return false;
  }

  std::string config_str;
  if (!base::WideToUTF8(config, wcslen(config), &config_str)) {
    VLOG(1) << "Unable to convert config to std::string. Is config valid?";
    return false;
  }

  // Check for invalid characters
  // low ascii (excluding CR/LF), #, high ascii
  if (re2::RE2::PartialMatch(
          config_str,
          R"([\x00-\x09]|[\x0b-\x0c]|[\x0d-\x1f]|[\x23]|[\x7f-\xFF])")) {
    VLOG(1) << "Config contains invalid characters.";
    return false;
  }

  return IsValidConfigEndpoint(config_str) && IsValidConfigDNS(config_str);
}

}  // namespace wireguard

}  // namespace brave_vpn

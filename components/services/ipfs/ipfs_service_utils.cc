/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ipfs/ipfs_service_utils.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#include "third_party/re2/src/re2/re2.h"

namespace {

// RegEx to extract version from node name
constexpr char kExecutableRegEx[] =
    "go-ipfs_v(\\d+\\.\\d+\\.\\d+)(-rc\\d+)?\\_\\w+-\\w+";

}  // namespace

namespace ipfs {

// Updates the ipfs node config to meet current preferences
bool UpdateConfigJSON(const std::string& source,
                      const ipfs::mojom::IpfsConfig* config,
                      std::string* result) {
  std::optional<base::Value> records_v = base::JSONReader::Read(
      source, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                  base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << source;
    return false;
  }
  base::Value::Dict* dict = records_v->GetIfDict();
  if (!dict) {
    VLOG(1) << "Could not parse JSON, JSON is: " << source;
    return false;
  }
  if (config->blessed_extension_list &&
      !config->blessed_extension_list->empty()) {
    base::Value::List origins;
    for (const auto& blessed_item : config->blessed_extension_list.value()) {
      origins.Append(blessed_item);
    }
    dict->SetByDottedPath("API.HTTPHeaders.Access-Control-Allow-Origin",
                          std::move(origins));
  }
  dict->SetByDottedPath(
      "Addresses.API", base::StrCat({"/ip4/127.0.0.1/tcp/", config->api_port}));
  dict->SetByDottedPath(
      "Addresses.Gateway",
      base::StrCat({"/ip4/127.0.0.1/tcp/", config->gateway_port}));
  dict->SetByDottedPath("Datastore.GCPeriod", "1h");
  dict->SetByDottedPath("Datastore.StorageMax", config->storage_max);

  base::Value::Dict localhost_gateway_settings;
  localhost_gateway_settings.Set("UseSubdomains", true);
  localhost_gateway_settings.Set("InlineDNSLink", true);
  base::Value::List path_list;
  path_list.Append("/ipfs");
  path_list.Append("/ipns");
  path_list.Append("/api");
  localhost_gateway_settings.Set("Paths", std::move(path_list));
  base::Value::Dict public_gateways;
  public_gateways.Set("localhost", std::move(localhost_gateway_settings));
  dict->SetByDottedPath("Gateway.PublicGateways", std::move(public_gateways));

  if (config->doh_server_url) {
    base::Value::Dict dns_resolvers;
    std::string doh_url = *(config->doh_server_url);
    // Kubo doesn't support RFC-8484 DOH url format
    base::ReplaceSubstringsAfterOffset(&doh_url, 0, "{?dns}", "");
    dns_resolvers.Set(".", std::move(doh_url));
    dict->SetByDottedPath("DNS.Resolvers", std::move(dns_resolvers));
  } else {
    dict->RemoveByDottedPath("DNS.Resolvers");
  }

  base::Value::List list;
  list.Append(base::StrCat({"/ip4/0.0.0.0/tcp/", config->swarm_port}));
  list.Append(base::ReplaceStringPlaceholders(
      "/ip4/0.0.0.0/udp/$1/quic-v1/webtransport", {config->swarm_port},
      nullptr));
  list.Append(base::ReplaceStringPlaceholders("/ip4/0.0.0.0/udp/$1/quic-v1",
                                              {config->swarm_port}, nullptr));
  list.Append(base::ReplaceStringPlaceholders("/ip6/::/udp/$1/quic-v1",
                                              {config->swarm_port}, nullptr));
  list.Append(base::ReplaceStringPlaceholders(
      "/ip6/::/udp/$1/quic-v1/webtransport", {config->swarm_port}, nullptr));
  list.Append(base::StrCat({"/ip6/::/tcp/", config->swarm_port}));

  dict->SetByDottedPath("Addresses.Swarm", std::move(list));
  dict->RemoveByDottedPath("Swarm.ConnMgr");

  std::string json_string;
  if (!base::JSONWriter::Write(records_v.value(), &json_string) ||
      json_string.empty()) {
    return false;
  }
  *result = json_string;
  return true;
}

std::string GetVersionFromNodeFilename(const std::string& filename) {
  static const RE2 version_pattern(kExecutableRegEx);
  std::vector<std::string_view> matched_groups(
      version_pattern.NumberOfCapturingGroups() + 1);
  if (!version_pattern.Match(filename, 0, filename.size(), RE2::ANCHOR_START,
                             matched_groups.data(), matched_groups.size()) ||
      matched_groups.size() < 2) {
    return std::string();
  }
  return std::string(matched_groups[1]);
}

}  // namespace ipfs

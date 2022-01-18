/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ipfs/ipfs_service_utils.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
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
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          source, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                      base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << source;
    return false;
  }
  base::DictionaryValue* dict = nullptr;
  if (!records_v->GetAsDictionary(&dict)) {
    VLOG(1) << "Could not parse JSON, JSON is: " << source;
    return false;
  }
  dict->Set("Addresses.API", std::make_unique<base::Value>(
                                 "/ip4/127.0.0.1/tcp/" + config->api_port));
  dict->Set("Addresses.Gateway",
            std::make_unique<base::Value>("/ip4/127.0.0.1/tcp/" +
                                          config->gateway_port));
  dict->Set("Datastore.GCPeriod", std::make_unique<base::Value>("1h"));
  dict->Set("Swarm.ConnMgr.LowWater", std::make_unique<base::Value>(50));
  dict->Set("Swarm.ConnMgr.HighWater", std::make_unique<base::Value>(300));
  dict->Set("Datastore.StorageMax",
            std::make_unique<base::Value>(config->storage_max));
  std::unique_ptr<base::ListValue> list = std::make_unique<base::ListValue>();
  list->Append(base::Value("/ip4/0.0.0.0/tcp/" + config->swarm_port));
  list->Append(base::Value("/ip6/::/tcp/" + config->swarm_port));
  dict->Set("Addresses.Swarm", std::move(list));
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
  std::vector<re2::StringPiece> matched_groups(
      version_pattern.NumberOfCapturingGroups() + 1);
  if (!version_pattern.Match(filename, 0, filename.size(), RE2::ANCHOR_START,
                             matched_groups.data(), matched_groups.size()) ||
      matched_groups.size() < 2) {
    return std::string();
  }
  return matched_groups[1].as_string();
}

}  // namespace ipfs

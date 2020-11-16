/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/ipfs/ipfs_json_parser.h"

#include "base/json/json_reader.h"
#include "base/logging.h"

// static
// Response Format for /api/v0/swarm/peers
// {
//    "Peers": [
//      {
//        "Addr": "<string>",
//        "Direction": "<int>",
//        "Latency": "<string>",
//        "Muxer": "<string>",
//        "Peer": "<string>",
//        "Streams": [
//          {
//            "Protocol": "<string>"
//          }
//        ]
//      }
//    ]
// }
bool IPFSJSONParser::GetPeersFromJSON(const std::string& json,
                                      std::vector<std::string>* peers) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* peers_arr = records_v->FindKey("Peers");
  if (!peers_arr || !peers_arr->is_list()) {
    VLOG(1) << "Invalid response, can not find Peers array.";
    return false;
  }

  for (const base::Value& val : peers_arr->GetList()) {
    const base::Value* addr = val.FindKey("Addr");
    const base::Value* peer = val.FindKey("Peer");

    bool has_addr = addr && addr->is_string();
    bool has_peer = peer && peer->is_string();

    if (!has_addr || !has_peer) {
      continue;
    }

    peers->push_back(addr->GetString() + "/p2p/" + peer->GetString());
  }

  return true;
}

// static
// Response Format for /api/v0/config?arg=Addresses
// {
//    "Key": "Addresses",
//    "Value":
//      {
//        "API": "<string>",
//        "Announce": [],
//        "Gateway": "<int>",
//        "NoAnnounce":[],
//        "Swarm": [
//          <string>
//        ]
//      }
// }
bool IPFSJSONParser::GetAddressesConfigFromJSON(const std::string& json,
                                                ipfs::AddressesConfig* config) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return false;
  }

  const base::DictionaryValue* val_dict;
  if (!response_dict->GetDictionary("Value", &val_dict)) {
    return false;
  }

  const std::string* api = val_dict->FindStringKey("API");
  const std::string* gateway = val_dict->FindStringKey("Gateway");
  const base::Value* swarm = val_dict->FindListKey("Swarm");
  if (!api || !gateway || !swarm) {
    VLOG(1) << "Invalid response, missing required keys in value dictionary.";
    return false;
  }

  config->api = *api;
  config->gateway = *gateway;
  for (const base::Value& val : swarm->GetList()) {
    if (!val.is_string()) {
      continue;
    }

    config->swarm.push_back(val.GetString());
  }

  return true;
}

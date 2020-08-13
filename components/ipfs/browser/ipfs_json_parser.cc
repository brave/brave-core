/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/components/ipfs/browser/ipfs_json_parser.h"

#include "base/json/json_reader.h"

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
bool IPFSJSONParser::GetPeersFromJSON(
    const std::string& json, std::vector<std::string>& peers) {
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

  for (const base::Value &val : peers_arr->GetList()) {
    const base::Value* addr = val.FindKey("Addr");
    const base::Value* peer = val.FindKey("Peer");

    bool has_addr = addr && addr->is_string();
    bool has_peer = peer && peer->is_string();

    if (!has_addr || !has_peer) {
      continue;
    }

    peers.push_back(addr->GetString() + "/" + peer->GetString());
  }

  return true;
}

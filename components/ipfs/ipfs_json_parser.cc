/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <set>
#include <utility>
#include <vector>

#include "brave/components/ipfs/ipfs_json_parser.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

base::Value* FindPeerElement(base::Value* root, const std::string& peer_id) {
  for (base::Value& val : root->GetList()) {
    base::Value* id = val.FindKey("ID");
    bool has_peer = id && id->is_string();
    if (!has_peer)
      continue;

    if (id->GetString() != peer_id)
      continue;
    return &val;
  }
  return nullptr;
}

bool RemoveElementFromList(base::Value* root,
                           const base::Value& item_to_remove) {
  DCHECK(root);
  base::ListValue* list = nullptr;
  if (!root->GetAsList(&list) || !list)
    return false;
  return list->EraseListValue(item_to_remove);
}

bool RemoveValueFromList(base::Value* root,
                         const std::string& value_to_remove) {
  DCHECK(root);
  for (base::Value& item : root->GetList()) {
    auto current = item.GetString();
    if (current != value_to_remove)
      continue;
    return RemoveElementFromList(root, item);
  }
  return false;
}

}  // namespace

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
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

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
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

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

// static
// Response Format for /api/v0/repo/stat
//{
//  "NumObjects": "<uint64>",
//  "RepoPath": "<string>",
//  "RepoSize": "<uint64>",
//  "StorageMax": "<uint64>",
//  "Version": "<string>"
//}
bool IPFSJSONParser::GetRepoStatsFromJSON(const std::string& json,
                                          ipfs::RepoStats* stats) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto num_objects_value = response_dict->FindDoubleKey("NumObjects");
  const auto size_value = response_dict->FindDoubleKey("RepoSize");
  const auto storage_max_value = response_dict->FindDoubleKey("StorageMax");
  const std::string* path = response_dict->FindStringKey("RepoPath");
  const std::string* version = response_dict->FindStringKey("Version");

  if (!num_objects_value || !size_value || !storage_max_value || !path ||
      !version) {
    VLOG(1) << "Invalid response, missing required keys in value dictionary.";
    return false;
  }

  stats->objects = static_cast<uint64_t>(*num_objects_value);
  stats->size = static_cast<uint64_t>(*size_value);
  stats->storage_max = static_cast<uint64_t>(*storage_max_value);
  stats->path = *path;
  stats->version = *version;
  return true;
}

// static
// Response Format for /api/v0/id
//{
//  "Addresses": ["<string>"],
//  "AgentVersion": "<string>",
//  "ID": "<string>",
//  "ProtocolVersion": "<string>",
//  "Protocols": ["<string>"],
//  "PublicKey": "<string>"
//}
bool IPFSJSONParser::GetNodeInfoFromJSON(const std::string& json,
                                         ipfs::NodeInfo* info) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* version = response_dict->FindStringKey("AgentVersion");
  const std::string* peerid = response_dict->FindStringKey("ID");

  if (!peerid || !version) {
    VLOG(1) << "Invalid response, missing required keys in value dictionary.";
    return false;
  }

  info->id = *peerid;
  info->version = *version;
  return true;
}

// static
// Response Format for /api/v0/repo/gc
// {
//   "Error": "<string>",
//   "Key": {
//     "/": "<cid-string>"
//   }
// }
bool IPFSJSONParser::GetGarbageCollectionFromJSON(const std::string& json,
                                                  std::string* error) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << value_with_error.error_message;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* error_message = response_dict->FindStringKey("Error");
  if (error_message)
    *error = *error_message;
  return true;
}

// static
// Response Format for /api/v0/add
// {
//   \"Name\":\"yandex.ru\",
//   \"Hash\":\"QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU\",
//   \"Size\":\"567857\"
// }
bool IPFSJSONParser::GetImportResponseFromJSON(const std::string& json,
                                               ipfs::ImportedData* data) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << value_with_error.error_message;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* name = response_dict->FindStringKey("Name");
  if (name)
    data->filename = *name;

  const std::string* hash = response_dict->FindStringKey("Hash");
  if (hash)
    data->hash = *hash;

  const std::string* size_value = response_dict->FindStringKey("Size");
  if (size_value)
    data->size = std::stoll(*size_value);
  return true;
}

// static
// Response Format for /api/v0/key/list
// {"Keys" : [
//   {"Name":"self","Id":"k51q...wal"}
// ]}
bool IPFSJSONParser::GetParseKeysFromJSON(
    const std::string& json,
    std::unordered_map<std::string, std::string>* data) {
  DCHECK(data);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << value_with_error.error_message;
    return false;
  }

  const base::Value* list = records_v->FindListKey("Keys");
  if (!list) {
    VLOG(1) << "Invalid response, missing required keys in value dictionary.";
    return false;
  }
  auto& keys = *data;
  for (const base::Value& val : list->GetList()) {
    if (!val.is_dict()) {
      continue;
    }
    const std::string* name = val.FindStringKey("Name");
    const std::string* id = val.FindStringKey("Id");
    if (!name || !id)
      continue;

    keys[*name] = *id;
  }
  return true;
}

// static
// Response Format for /api/v0/key/gen
// {"Name":"self","Id":"k51q...wal"}
bool IPFSJSONParser::GetParseSingleKeyFromJSON(const std::string& json,
                                               std::string* name,
                                               std::string* value) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << value_with_error.error_message;
    return false;
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict) || !response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* key_name = response_dict->FindStringKey("Name");
  const std::string* key_id = response_dict->FindStringKey("Id");
  if (!key_name || !key_id)
    return false;
  *name = *key_name;
  *value = *key_id;
  return true;
}

// static
// Puts new adress for existing peer or adds a new peer to config
// https://github.com/ipfs/go-ipfs/blob/master/docs/config.md#peering
std::string IPFSJSONParser::PutNewPeerToConfigJSON(const std::string& json,
                                                   const std::string& peer) {
  std::string peer_id;
  std::string address;
  if (!ipfs::ParsePeerConnectionString(peer, &peer_id, &address))
    return std::string();

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return std::string();
  }
  base::Value* peering = records_v->FindKey("Peering");
  if (!peering) {
    records_v->SetKey("Peering", base::Value(base::Value::Type::DICTIONARY));
    peering = records_v->FindKey("Peering");
  }
  DCHECK(peering);
  base::Value* peers_arr = peering->FindListKey("Peers");
  if (!peers_arr) {
    base::Value item(base::Value::Type::LIST);
    peering->SetPath("Peers", std::move(item));
    peers_arr = peering->FindListKey("Peers");
  }
  DCHECK(peers_arr);
  DCHECK(peers_arr->is_list());
  base::Value* peer_to_update = FindPeerElement(peers_arr, peer_id);
  if (!peer_to_update) {
    base::Value item(base::Value::Type::DICTIONARY);
    item.SetKey("ID", base::Value(peer_id));
    peers_arr->Append(std::move(item));
    peer_to_update = FindPeerElement(peers_arr, peer_id);
  }
  DCHECK(peer_to_update);
  if (!address.empty()) {
    base::Value item(address);
    base::Value* addresses = peer_to_update->FindListKey("Addrs");
    if (addresses) {
      addresses->Append(std::move(item));
    } else {
      base::Value list(base::Value::Type::LIST);
      list.Append(std::move(item));
      peer_to_update->SetKey("Addrs", std::move(list));
    }
  }

  std::string json_string;
  base::JSONWriter::Write(records_v.value(), &json_string);
  return json_string;
}

// static
// Gets peer list from Peering.Peers config
// https://github.com/ipfs/go-ipfs/blob/master/docs/config.md#peering
bool IPFSJSONParser::GetPeersFromConfigJSON(const std::string& json,
                                            std::vector<std::string>* peers) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return false;
  }
  const base::Value* peers_arr = records_v->FindPath("Peering.Peers");
  if (!peers_arr || !peers_arr->is_list()) {
    VLOG(1) << "Invalid json, can not find Peers array.";
    return false;
  }

  for (const base::Value& val : peers_arr->GetList()) {
    const base::Value* peer = val.FindKey("ID");
    const base::Value* addr = val.FindListKey("Addrs");

    bool has_addr = addr && addr->is_list() && !addr->GetList().empty();
    bool has_peer = peer && peer->is_string();
    if (!has_addr && !has_peer) {
      continue;
    }
    auto peer_id = peer->GetString();
    std::string value;
    if (has_addr) {
      for (const base::Value& item : addr->GetList()) {
        auto address = item.GetString();
        if (address.empty())
          continue;
        auto value = address + "/p2p/" + peer_id;
        peers->push_back(value);
      }
    } else {
      peers->push_back(peer_id);
    }
  }
  return true;
}

// static
// Removes a peer or peer address from Peering.Peers config
// https://github.com/ipfs/go-ipfs/blob/master/docs/config.md#peering
std::string IPFSJSONParser::RemovePeerFromConfigJSON(
    const std::string& json,
    const std::string& peer_id,
    const std::string& peer_address) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return std::string();
  }
  base::Value* peers_arr = records_v->FindPath("Peering.Peers");
  if (!peers_arr || !peers_arr->is_list())
    return json;
  base::Value* peer_to_update = FindPeerElement(peers_arr, peer_id);
  if (!peer_to_update)
    return json;
  DCHECK(peer_to_update);
  if (!peer_address.empty()) {
    base::Value* addresses = peer_to_update->FindListKey("Addrs");
    if (!addresses)
      return json;
    if (!RemoveValueFromList(addresses, peer_address))
      return json;
    if (addresses->GetList().empty())
      RemoveElementFromList(peers_arr, *peer_to_update);
  } else {
    RemoveElementFromList(peers_arr, *peer_to_update);
  }

  std::string json_string;
  base::JSONWriter::Write(records_v.value(), &json_string);
  return json_string;
}

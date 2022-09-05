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
#include "base/strings/strcat.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

base::Value::Dict* FindPeerElement(base::Value::List* root,
                                   const std::string& peer_id) {
  for (base::Value& val : *root) {
    auto& dict = val.GetDict();
    const auto* id = dict.FindString("ID");
    if (!id || *id != peer_id)
      continue;
    return &dict;
  }
  return nullptr;
}

template <typename T>
bool RemoveValueFromList(base::Value::List* root, const T& value_to_remove) {
  DCHECK(root);
  return root->EraseIf([&value_to_remove](const auto& value) {
    return value == value_to_remove;
  });
}

}  // namespace

// static
// Response format /api/v0/pin/add
// {
//   "Pins": [
//     "<string>"
//   ],
//   "Progress": "<int>"
// }
bool IPFSJSONParser::GetAddPinsResultFromJSON(
    const std::string& json,
    ipfs::AddPinResult* add_pin_result) {
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* pins_arr = records_v->FindListKey("Pins");
  if (!pins_arr || !pins_arr->is_list()) {
    VLOG(1) << "Invalid response, can not find Pins array.";
    return false;
  }

  auto progress = records_v->FindIntKey("Progress");
  if (progress) {
    add_pin_result->progress = *progress;
  } else {
    add_pin_result->progress = -1;
  }

  for (const base::Value& val : pins_arr->GetList()) {
    add_pin_result->pins.push_back(val.GetString());
  }
  return true;
}

// static
// Response format /api/v0/pin/rm
// {
//   "Pins": [
//     "<string>"
//   ]
// }
bool IPFSJSONParser::GetRemovePinsResultFromJSON(
    const std::string& json,
    ipfs::RemovePinResult* remove_pin_result) {
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* pins_arr = records_v->FindKey("Pins");
  if (!pins_arr || !pins_arr->is_list()) {
    VLOG(1) << "Invalid response, can not find Pins array.";
    return false;
  }

  ipfs::RemovePinResult result;
  for (const base::Value& val : pins_arr->GetList()) {
    auto* val_as_str = val.GetIfString();
    if (!val_as_str) {
      return false;
    }
    result.push_back(*val_as_str);
  }
  *remove_pin_result = result;
  return true;
}

// static
// Response format /api/v0/pin/ls
// {
//   "PinLsList": {
//     "Keys": {
//       "<string>": {
//         "Type": "<string>"
//       }
//     }
//   },
//   "PinLsObject": {
//     "Cid": "<string>",
//     "Type": "<string>"
//   }
// }
bool IPFSJSONParser::GetGetPinsResultFromJSON(
    const std::string& json,
    ipfs::GetPinsResult* get_pins_result) {
  DCHECK(get_pins_result);
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const base::Value* pins_ls_list = records_v->FindKey("PinLsList");
  const base::Value* pins_ls_object = records_v->FindKey("PinLsObject");

  if ((!pins_ls_list || !pins_ls_list->is_dict()) &&
      (!pins_ls_object || !pins_ls_object->is_dict())) {
    VLOG(1)
        << "Invalid response, can not find PinLsList and PinLsObject dicts.";
    return false;
  }

  if (pins_ls_list) {
    const base::Value* keys = pins_ls_list->FindKey("Keys");
    if (!keys || !keys->is_dict()) {
      VLOG(1) << "Invalid response, can not find Keys in PinLsList dict.";
      return false;
    }

    for (const auto it : keys->GetDict()) {
      const std::string* type = it.second.FindStringKey("Type");
      if (!type) {
        VLOG(1) << "Missing Type for " << it.first;
        return false;
      }
      (*get_pins_result)[it.first] = *type;
    }
  } else {
    const std::string* cid = pins_ls_object->FindStringKey("Cid");
    const std::string* value = pins_ls_object->FindStringKey("Type");
    if (!cid || !value) {
      VLOG(1) << "Invalid PinLsObject format.";
      return false;
    }
    (*get_pins_result)[*cid] = *value;
  }
  return true;
}

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
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v || !records_v->is_dict()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  auto& dict = records_v->GetDict();
  const auto* peers_arr = dict.FindList("Peers");
  if (!peers_arr) {
    VLOG(1) << "Invalid response, can not find Peers array.";
    return false;
  }

  for (const base::Value& item : *peers_arr) {
    const auto& val = item.GetDict();
    const auto* addr = val.FindString("Addr");
    const auto* peer = val.FindString("Peer");

    if (!addr || !peer) {
      continue;
    }

    peers->push_back(base::StrCat({*addr, "/p2p/", *peer}));
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
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    return false;
  }

  const auto* val_dict = response_dict->FindDict("Value");
  if (!val_dict) {
    return false;
  }

  const std::string* api = val_dict->FindString("API");
  const std::string* gateway = val_dict->FindString("Gateway");
  const auto* swarm = val_dict->FindList("Swarm");
  if (!api || !gateway || !swarm) {
    VLOG(1) << "Invalid response, missing required keys in value dictionary.";
    return false;
  }

  config->api = *api;
  config->gateway = *gateway;
  for (const base::Value& val : *swarm) {
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
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto num_objects_value = response_dict->FindDouble("NumObjects");
  const auto size_value = response_dict->FindDouble("RepoSize");
  const auto storage_max_value = response_dict->FindDouble("StorageMax");
  const std::string* path = response_dict->FindString("RepoPath");
  const std::string* version = response_dict->FindString("Version");

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
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* version = response_dict->FindString("AgentVersion");
  const std::string* peerid = response_dict->FindString("ID");

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
  auto records_v = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v.has_value()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << records_v.error().message;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* error_message = response_dict->FindString("Error");
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
  auto records_v = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v.has_value()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << records_v.error().message;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* name = response_dict->FindString("Name");
  if (name)
    data->filename = *name;

  const std::string* hash = response_dict->FindString("Hash");
  if (hash)
    data->hash = *hash;

  const std::string* size_value = response_dict->FindString("Size");
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
  auto records_v = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v.has_value()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << records_v.error().message;
    return false;
  }

  const auto* dict = records_v->GetIfDict();
  if (!dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* list = dict->FindList("Keys");
  if (!list) {
    VLOG(1) << "Invalid response, missing required keys in value dictionary.";
    return false;
  }
  auto& keys = *data;
  for (const base::Value& item : *list) {
    const auto* val = item.GetIfDict();
    if (!val) {
      continue;
    }
    const std::string* name = val->FindString("Name");
    const std::string* id = val->FindString("Id");
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
  auto records_v = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v.has_value()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json
            << " error is:" << records_v.error().message;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* key_name = response_dict->FindString("Name");
  const std::string* key_id = response_dict->FindString("Id");
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

  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return std::string();
  }

  auto* dict = records_v->GetIfDict();
  if (!dict) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return std::string();
  }

  base::Value::Dict* peering = dict->FindDict("Peering");
  if (!peering) {
    peering = dict->Set("Peering", base::Value::Dict())->GetIfDict();
  }
  DCHECK(peering);
  base::Value::List* peers_arr = peering->FindList("Peers");
  if (!peers_arr) {
    peers_arr = peering->Set("Peers", base::Value::List())->GetIfList();
  }
  DCHECK(peers_arr);
  base::Value::Dict* peer_to_update = FindPeerElement(peers_arr, peer_id);
  if (!peer_to_update) {
    base::Value::Dict item;
    item.Set("ID", peer_id);
    peers_arr->Append(std::move(item));
    peer_to_update = FindPeerElement(peers_arr, peer_id);
  }
  DCHECK(peer_to_update);
  if (!address.empty()) {
    auto* addresses = peer_to_update->FindList("Addrs");
    if (addresses) {
      addresses->Append(address);
    } else {
      base::Value::List list;
      list.Append(address);
      peer_to_update->Set("Addrs", std::move(list));
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
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* dict = records_v->GetIfDict();
  if (!dict) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* peers_arr = dict->FindListByDottedPath("Peering.Peers");
  if (!peers_arr) {
    VLOG(1) << "Invalid json, can not find Peers array.";
    return false;
  }

  for (const base::Value& item : *peers_arr) {
    const auto& val = item.GetDict();
    const auto* peer = val.FindString("ID");
    const auto* addr = val.FindList("Addrs");

    bool has_addr = addr && !addr->empty();
    if (!peer) {
      continue;
    }
    if (has_addr) {
      for (const base::Value& addr_item : *addr) {
        auto address = addr_item.GetString();
        if (address.empty())
          continue;
        peers->push_back(base::StrCat({address, "/p2p/", *peer}));
      }
    } else {
      peers->push_back(*peer);
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
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return std::string();
  }

  auto* dict = records_v->GetIfDict();
  if (!dict) {
    VLOG(1) << "Could not parse JSON, JSON is: " << json;
    return std::string();
  }

  base::Value::List* peers_arr = dict->FindListByDottedPath("Peering.Peers");
  if (!peers_arr)
    return json;
  base::Value::Dict* peer_to_update = FindPeerElement(peers_arr, peer_id);
  if (!peer_to_update)
    return json;
  if (!peer_address.empty()) {
    base::Value::List* addresses = peer_to_update->FindList("Addrs");
    if (!addresses)
      return json;
    if (!RemoveValueFromList(addresses, base::Value(peer_address)))
      return json;
    if (addresses->empty())
      RemoveValueFromList(peers_arr, *peer_to_update);
  } else {
    RemoveValueFromList(peers_arr, *peer_to_update);
  }

  std::string json_string;
  base::JSONWriter::Write(records_v.value(), &json_string);
  return json_string;
}

// static
bool IPFSJSONParser::GetGetRemotePinServicesResult(
    const std::string& json,
    ipfs::GetRemotePinServicesResult* result) {
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  LOG(ERROR) << "XXXZZZ get remote pin services result " << json;
  if (!records_v.has_value()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  LOG(ERROR) << "XXXZZZ 1";

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  LOG(ERROR) << "XXXZZZ 2";

  const base::Value::List* list = response_dict->FindList("RemoteServices");
  if (!list) {
    VLOG(1) << "Invalid response, wrong JSON format: " << json;
    return false;
  }
  LOG(ERROR) << "XXXZZZ 3";

  std::vector<ipfs::RemotePinServiceItem> items;
  for (const base::Value& value : *list) {
    ipfs::RemotePinServiceItem item;

    const std::string* endpoint = value.FindStringKey("ApiEndpoint");
    const std::string* service = value.FindStringKey("Service");
    const std::string* status = value.FindStringKey("Status");

    if (!endpoint || !service) {
      VLOG(1) << "Invalid response, wrong JSON format: " << json;
      return false;
    }

    item.api_endpoint = *endpoint;
    item.service = *service;
    if (status) {
      item.status = *status;
    }

    items.push_back(item);
  }

  result->remote_services = items;

  return true;
}

// static
bool IPFSJSONParser::GetAddRemotePinResultFromJson(
    const std::string& json,
    ipfs::AddRemotePinResult* result) {
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v.has_value()) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }

  const auto* response_dict = records_v->GetIfDict();
  if (!response_dict) {
    VLOG(1) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const std::string* key_name = response_dict->FindString("Name");
  const std::string* key_cid = response_dict->FindString("Cid");
  const std::string* key_status = response_dict->FindString("Status");
  if (!key_name || !key_cid || !key_status)
    return false;
  result->name = *key_name;
  result->cid = *key_cid;
  result->status = *key_status;
  return true;
}

// static
bool IPFSJSONParser::GetGetRemotePinsResultFromJson(
    const std::string& json,
    ipfs::GetRemotePinResult* result) {
  return true;
}

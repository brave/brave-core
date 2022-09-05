/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_PIN_RFC_TYPES_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_PIN_RFC_TYPES_H_

#include <map>
#include <string>
#include <vector>

namespace ipfs {

struct RemotePinStatus {
  std::string cid;
  std::string name;
  std::string status;

  RemotePinStatus();
  ~RemotePinStatus();
};

struct AddRemotePinResult {
  AddRemotePinResult();
  ~AddRemotePinResult();

  std::string cid;
  std::string name;
  std::string status;
};

struct GetRemotePinResult {
  GetRemotePinResult();
  ~GetRemotePinResult();
  GetRemotePinResult(const GetRemotePinResult& result);

  std::vector<RemotePinStatus> statuses;
};

struct RemotePinServiceItem {
  RemotePinServiceItem();
  ~RemotePinServiceItem();
  RemotePinServiceItem(const RemotePinServiceItem&);

  std::string api_endpoint;
  std::string service;
  std::string status;
};

struct GetRemotePinServicesResult {
  GetRemotePinServicesResult();
  ~GetRemotePinServicesResult();
  GetRemotePinServicesResult(const GetRemotePinServicesResult&);
  std::vector<RemotePinServiceItem> remote_services;
};

struct AddPinResult {
  AddPinResult();
  ~AddPinResult();
  AddPinResult(const AddPinResult&);
  std::vector<std::string> pins;
  int progress;
};

using GetPinsResult = std::map<std::string, std::string>;

using RemovePinResult = std::vector<std::string>;

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_PIN_RFC_TYPES_H_

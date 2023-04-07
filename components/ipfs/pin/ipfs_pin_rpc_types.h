// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_PIN_RPC_TYPES_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_PIN_RPC_TYPES_H_

#include <map>
#include <string>
#include <vector>

namespace ipfs {

struct AddPinResult {
  AddPinResult();
  ~AddPinResult();
  AddPinResult(const AddPinResult&);
  AddPinResult& operator=(const AddPinResult&);
  bool recursive = true;
  std::vector<std::string> pins;
  int progress = 0;
};

using GetPinsResult = std::map<std::string, std::string>;

using RemovePinResult = std::vector<std::string>;

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_PIN_RPC_TYPES_H_

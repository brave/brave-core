// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_pin_rpc_types.h"

namespace ipfs {

AddPinResult::AddPinResult() = default;

AddPinResult::~AddPinResult() = default;

AddPinResult::AddPinResult(const AddPinResult& other) = default;

AddPinResult& AddPinResult::operator=(const AddPinResult&) = default;

}  // namespace ipfs

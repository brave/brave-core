/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_GLOBAL_CONSTANTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_GLOBAL_CONSTANTS_H_

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "bat/ledger/export.h"

namespace ledger {
namespace constant {

const char kWalletAnonymous[] = "anonymous";
const char kWalletUphold[] = "uphold";
const char kWalletUnBlinded[] = "blinded";
const char kWalletBitflyer[] = "bitflyer";

}  // namespace constant
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_GLOBAL_CONSTANTS_H_

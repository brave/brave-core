/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_H_
#define BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_H_

#include "bat/ledger/ledger_client.h"
#include "bat/confirmations/export.h"
#include "bat/confirmations/wallet_info.h"

namespace confirmations {

// backwards compat for now
using URLRequestMethod = ::ledger::UrlMethod;
using Result = ::ledger::Result;
const auto SUCCESS = ::ledger::Result::LEDGER_OK;
const auto FAILED = ::ledger::Result::LEDGER_ERROR;

using OnSaveCallback = ::ledger::OnSaveCallback;
using OnLoadCallback = ::ledger::OnLoadCallback;
using OnResetCallback = ::ledger::OnResetCallback;
using URLRequestCallback = ::ledger::LoadURLCallback;

using ConfirmationsClient = ::ledger::LedgerClient;

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_CLIENT_H_

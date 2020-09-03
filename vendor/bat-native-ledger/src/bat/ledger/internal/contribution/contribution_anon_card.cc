/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ledger/internal/contribution/contribution_anon_card.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/sku/sku_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace contribution {

ContributionAnonCard::ContributionAnonCard(LedgerImpl* ledger) :
    ledger_(ledger),
    payment_server_(std::make_unique<endpoint::PaymentServer>(ledger)) {
  DCHECK(ledger_);
}

ContributionAnonCard::~ContributionAnonCard() = default;

void ContributionAnonCard::SendTransaction(
    const double amount,
    const std::string& order_id,
    const std::string& destination,
    client::TransactionCallback callback) {
  auto url_callback = std::bind(&ContributionAnonCard::OnSendTransaction,
      this,
      _1,
      callback);

  payment_server_->post_transaction_anon()->Request(
      amount,
      order_id,
      destination,
      url_callback);
}

void ContributionAnonCard::OnSendTransaction(
    const type::Result result,
    client::TransactionCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Problem sending transaction");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  callback(type::Result::LEDGER_OK, "");
}

}  // namespace contribution
}  // namespace ledger

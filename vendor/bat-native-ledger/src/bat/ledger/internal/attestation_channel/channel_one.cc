/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation_channel/channel_one.h"
#include "bat/ledger/internal/request/private_channel_requests.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_attestation_channel {

  PrivateChannelOne::PrivateChannelOne(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    attestation_timer_id_(0u) {
    }

  PrivateChannelOne::~PrivateChannelOne() {
  }

  void PrivateChannelOne::Initialize() {
    auto start_timer_in = timers[1];
    SetTimer(&attestation_timer_id_, start_timer_in);
  }
  
  void PrivateChannelOne::SetTimer(uint32_t* timer_id, uint64_t start_timer_in){
    ledger_->SetTimer(start_timer_in, timer_id);
  }

  void PrivateChannelOne::OnTimer(uint32_t timer_id) {
    auto start_timer_in = timers[1];
    SetTimer(&attestation_timer_id_, start_timer_in);

    StartProtocol();
}

// first protocol round callback 
void PrivateChannelOne::OnFirstRoundResponse(
  int response_status_code,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {
  }

// second protocol round callback 
void PrivateChannelOne::OnSecondRoundResponse(
  int response_status_code,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {
}

void PrivateChannelOne::StartProtocol() {
  std::string payload;
  const std::string url = braveledger_request_util::GetStartProtocolUrl();

  auto url_callback = std::bind(&PrivateChannelOne::OnFirstRoundResponse,
      this,
      _1,
      _2,
      _3);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::PUT,
      url_callback);
}


} // braveledger_attestation_channel

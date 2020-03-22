/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation_channel/channel_one.h"
#include "bat/ledger/internal/request/private_channel_requests.h"
#include "bat/ledger/internal/ledger_impl.h"

#include "brave/vendor/client_attestation_ffi/src/wrapper.hpp" // NOLINT

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

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "PrivateChannelOne::OnFirstRoundResponse";

  SecondRoundProtocol();
  }

// second protocol round callback 
void PrivateChannelOne::OnSecondRoundResponse(
  int response_status_code,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "PrivateChannelOne::OnSecondRoundResponse";

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "====== DONE =======";

}

void PrivateChannelOne::StartProtocol() {

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "PrivateChannelOne::StartProtocol";

  std::string sig = "signal";
  const char* input[] = { sig.c_str() };
  int input_size = sizeof(input)/sizeof(input[0]);

  const uint8_t server_pk_encoded[] = {78, 181, 75, 245, 70, 218, 146, 152, 155, 118, 20, 184, 203, 179, 192, 222, 212, 79, 178, 76, 232, 250, 218, 196, 6, 254, 139, 145, 172,18, 189, 13};

  auto artifacts = client_attestation::start_challenge(input, input_size, server_pk_encoded); 

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "PrivateChannelOne::RUNNING::"
  << std::endl
  << artifacts.error
  << std::endl
  << server_pk_encoded
  << std::endl
  << input
  << std::endl
  << input_size
  << std::endl
  << "============";

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

void PrivateChannelOne::SecondRoundProtocol() {

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "PrivateChannelOne::SecondRoundProtocol";

  std::string payload;
  const std::string url = braveledger_request_util::GetResultProtocolUrl();

  auto url_callback = std::bind(&PrivateChannelOne::OnSecondRoundResponse,
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

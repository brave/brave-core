/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/client_attestation/client_attestation_loader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/attestation_channel/channel_one.h"
#include "bat/ledger/internal/request/private_channel_requests.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_attestation_channel {

  PrivateChannelOne::PrivateChannelOne(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    server_pk(NULL),
    attestation_timer_id_(0u) {
    }

  PrivateChannelOne::~PrivateChannelOne() {
  }

  void PrivateChannelOne::Initialize(bool init_timer) {
    if (init_timer) {
      auto start_timer_in = timers[1];
      SetTimer(&attestation_timer_id_, start_timer_in);
    }

    auto url = braveledger_request_util::GetServerPublicKey();
    auto callback = std::bind(&PrivateChannelOne::OnServerPublicKeyResponse,
        this,
        _1,
        _2,
        _3);

    ledger_->LoadURL(
        url,
        std::vector<std::string>(),
        "",
        "application/json",
        ledger::UrlMethod::GET,
        callback);
  }
  
  void PrivateChannelOne::SetTimer(uint32_t* timer_id, uint64_t start_timer_in){
    ledger_->SetTimer(start_timer_in, timer_id);
  }

  void PrivateChannelOne::OnTimer(uint32_t timer_id) {
    auto start_timer_in = timers[1];
    SetTimer(&attestation_timer_id_, start_timer_in);

    StartProtocol();
}

void PrivateChannelOne::OnServerPublicKeyResponse(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {

  // TODO: use response payload instead of static value?
  //server_pk = response;
  server_pk = braveledger_ledger::PRIVATE_CHANNEL_SERVER_PK;

  ledger_->LogResponse(__func__, response_status_code, response, headers);

}

void PrivateChannelOne::OnFirstRoundResponse(
  std::string client_sk,
  std::string wallet_id,
  int input_size,
  int response_status_code,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  // TODO: Check status code

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) 
    << "PrivateChannelOne::OnFirstRoundResponse"
    << response;

  SecondRoundProtocol(response, client_sk, wallet_id, input_size);
  }

// second protocol round callback 
void PrivateChannelOne::OnSecondRoundResponse(
  int response_status_code,
  const std::string& response,
  const std::map<std::string, std::string>& headers) {

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) 
    << "PrivateChannelOne::OnSecondRoundResponse";

  ledger_->LogResponse(__func__, response_status_code, response, headers);
}

void PrivateChannelOne::StartProtocol() {

  // TODO: get signals
  std::string sig0 = "check1";
  std::string sig1 = "check2";
  std::string sig2 = "check3";

  const char* input[] = { sig0.c_str(), sig1.c_str(), sig2.c_str() };
  int input_size = sizeof(input)/sizeof(input[0]);

  auto request_artefacts = ChallengeFirstRound(input, input_size, server_pk);

  // TODO: get wallet ID
  std::string wallet_id = "wallet_id_mock";

  const std::string payload = base::StringPrintf("pk=%s&th_key=%s&enc_signals=%s&wallet_id=%s", 
      request_artefacts.client_pk.c_str(),
      request_artefacts.shared_pubkey.c_str(), 
      request_artefacts.encrypted_hashes.c_str(), 
      wallet_id.c_str()
    );

   BLOG(ledger_, ledger::LogLevel::LOG_ERROR) 
     << "PrivateChannelOne::StartProtocol::Payload\n"
     << payload;
     
  const std::string url = braveledger_request_util::GetStartProtocolUrl();
  auto url_callback = std::bind(&PrivateChannelOne::OnFirstRoundResponse,
      this,
      request_artefacts.client_sk,
      wallet_id,
      input_size,
      _1,
      _2,
      _3);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      payload,
      "application/x-www-form-urlencoded",
      ledger::UrlMethod::POST,
      url_callback);
}

void PrivateChannelOne::SecondRoundProtocol(
  const std::string& encrypted_input,
  std::string client_sk,
  std::string wallet_id,
  int input_size) {

  BLOG(ledger_, ledger::LogLevel::LOG_ERROR) 
    << encrypted_input << "\n"
    << client_sk << "\n"
    << wallet_id << "\n"
    << input_size << "\n\n";

  const char* _encrypted_input = encrypted_input.c_str();
  const char* _client_sk = &client_sk[0];

  auto request_artefacts = SecondRound(_encrypted_input, input_size, _client_sk);

   const std::string payload = base::StringPrintf("rand_vec=%s&partial_dec=%s&proofs=%s&wallet_id=%s", 
       request_artefacts.rand_vec.c_str(),
       request_artefacts.partial_decryption.c_str(),
       request_artefacts.proofs.c_str(),
       wallet_id.c_str()); 

   BLOG(ledger_, ledger::LogLevel::LOG_ERROR) 
     << "PrivateChannelOne::SecondRoundProtocol::Payload\n"
     << payload;

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
       "application/x-www-form-urlencoded",
       ledger::UrlMethod::POST,
       url_callback);
}

} // braveledger_attestation_channel

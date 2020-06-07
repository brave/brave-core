/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/private_channel/client_private_channel.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/private_channel/private_channel.h"
#include "bat/ledger/internal/request/request_private_channel.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_private_channel {

  PrivateChannel::PrivateChannel(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    server_pk(NULL),
    attestation_timer_id_(0u) {
    }

  PrivateChannel::~PrivateChannel() {
  }

  void PrivateChannel::Initialize(bool init_timer) {
    BLOG(1, "PrivateChannel::Initialize 1");

    if (init_timer) {
      auto start_timer_in = timers[0]; // 60s
      SetTimer(&attestation_timer_id_, start_timer_in);
    }

    auto url = braveledger_request_util::GetServerPublicKey();
    auto callback = std::bind(&PrivateChannel::OnServerPublicKeyResponse,
      this,
      _1);

    ledger_->LoadURL(
        url,
        std::vector<std::string>(),
        "",
        "application/json",
        ledger::UrlMethod::GET,
        callback);
  }
  
  void PrivateChannel::SetTimer(uint32_t* timer_id, uint64_t start_timer_in){
    ledger_->SetTimer(start_timer_in, timer_id);
  }

  void PrivateChannel::OnTimer(uint32_t timer_id) {
    auto start_timer_in = timers[1];
    SetTimer(&attestation_timer_id_, start_timer_in);

    StartProtocol();
}

void PrivateChannel::OnServerPublicKeyResponse(
    const ledger::UrlResponse& response) {    

  BLOG(1, "PrivateChannel::OnServerPublicKeyResponse");
  BLOG(1, ledger::UrlResponseToString(__func__, response));

  server_pk = braveledger_ledger::PRIVATE_CHANNEL_SERVER_PK;
}

void PrivateChannel::OnFirstRoundResponse(
  std::string client_sk,
  std::string wallet_id,
  int input_size,
  const ledger::UrlResponse& response) {

    BLOG(1, "PrivateChannel::OnFirstRoundResponse");
    BLOG(1, ledger::UrlResponseToString(__func__, response));

    if (response.status_code == net::HTTP_OK) {
      SecondRoundProtocol(response.body.c_str(), client_sk, wallet_id, input_size);
    }
  }

void PrivateChannel::OnSecondRoundResponse(
  const ledger::UrlResponse& response) {
  
  BLOG(1, "PrivateChannel::OnSecondRoundResponse");
  BLOG(1, ledger::UrlResponseToString(__func__, response));
}

void PrivateChannel::StartProtocol() {

  BLOG(0, "PrivateChannel::StartProtocol");

  // TODO: refactor and get signals 
  std::string sig0 = "check1";
  std::string sig1 = "check2";
  std::string sig2 = "check3";

  const char* input[] = { sig0.c_str(), sig1.c_str(), sig2.c_str() };
  int input_size = sizeof(input)/sizeof(input[0]);

  auto request_artefacts = ChallengeFirstRound(input, input_size, server_pk);

  // ## TODO
  //ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();
  //std::string wallet_id = wallet_info.payment_id;
  std::string wallet_id = "TODO:Ltest";

  const std::string payload = base::StringPrintf("pk=%s&th_key=%s&enc_signals=%s&client_id=%s", 
      request_artefacts.client_pk.c_str(),
      request_artefacts.shared_pubkey.c_str(), 
      request_artefacts.encrypted_hashes.c_str(), 
      wallet_id.c_str()
    );

  const std::string url = braveledger_request_util::GetStartProtocolUrl();
  auto url_callback = std::bind(&PrivateChannel::OnFirstRoundResponse,
      this,
      request_artefacts.client_sk,
      wallet_id,
      input_size,
      _1);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      payload,
      "application/x-www-form-urlencoded",
      ledger::UrlMethod::POST,
      url_callback);
}

void PrivateChannel::SecondRoundProtocol(
  const std::string& encrypted_input,
  std::string client_sk,
  std::string wallet_id,
  int input_size) {

  BLOG(1, "PrivateChannel::SecondRoundProtocol");
  
  const char* _encrypted_input = encrypted_input.c_str();
  const char* _client_sk = &client_sk[0];

  auto request_artefacts = SecondRound(_encrypted_input, input_size, _client_sk);

   const std::string payload = base::StringPrintf("rand_vec=%s&partial_dec=%s&proofs=%s&client_id=%s", 
       request_artefacts.rand_vec.c_str(),
       request_artefacts.partial_decryption.c_str(),
       request_artefacts.proofs.c_str(),
       wallet_id.c_str()); 

   const std::string url = braveledger_request_util::GetResultProtocolUrl();
    auto url_callback = std::bind(&PrivateChannel::OnSecondRoundResponse,
        this,
        _1);

   ledger_->LoadURL(
       url,
       std::vector<std::string>(),
       payload,
       "application/x-www-form-urlencoded",
       ledger::UrlMethod::POST,
       url_callback);
}

} // braveledger_private_channel

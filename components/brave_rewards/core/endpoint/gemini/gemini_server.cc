/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace endpoint {

GeminiServer::GeminiServer(LedgerImpl* ledger)
    : post_account_(std::make_unique<gemini::PostAccount>(ledger)),
      post_balance_(std::make_unique<gemini::PostBalance>(ledger)),
      post_oauth_(std::make_unique<gemini::PostOauth>(ledger)),
      post_recipient_id_(std::make_unique<gemini::PostRecipientId>(ledger)) {}

GeminiServer::~GeminiServer() = default;

gemini::PostAccount* GeminiServer::post_account() const {
  return post_account_.get();
}

gemini::PostBalance* GeminiServer::post_balance() const {
  return post_balance_.get();
}

gemini::PostOauth* GeminiServer::post_oauth() const {
  return post_oauth_.get();
}

gemini::PostRecipientId* GeminiServer::post_recipient_id() const {
  return post_recipient_id_.get();
}

}  // namespace endpoint
}  // namespace brave_rewards::core

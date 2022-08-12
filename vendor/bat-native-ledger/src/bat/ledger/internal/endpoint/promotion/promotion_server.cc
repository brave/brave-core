/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

PromotionServer::PromotionServer(LedgerImpl* ledger)
    : delete_claim_(std::make_unique<promotion::DeleteClaim>(ledger)),
      get_available_(std::make_unique<promotion::GetAvailable>(ledger)),
      post_creds_(std::make_unique<promotion::PostCreds>(ledger)),
      get_signed_creds_(std::make_unique<promotion::GetSignedCreds>(ledger)),
      post_clobbered_claims_(
          std::make_unique<promotion::PostClobberedClaims>(ledger)),
      post_bat_loss_(std::make_unique<promotion::PostBatLoss>(ledger)),
      post_wallet_brave_(std::make_unique<promotion::PostWalletBrave>(ledger)),
      get_recover_wallet_(
          std::make_unique<promotion::GetRecoverWallet>(ledger)),
      post_claim_bitflyer_(
          std::make_unique<promotion::PostClaimBitflyer>(ledger)),
      post_claim_gemini_(std::make_unique<promotion::PostClaimGemini>(ledger)),
      post_claim_uphold_(std::make_unique<promotion::PostClaimUphold>(ledger)),
      get_wallet_(std::make_unique<promotion::GetWallet>(ledger)),
      post_captcha_(std::make_unique<promotion::PostCaptcha>(ledger)),
      get_captcha_(std::make_unique<promotion::GetCaptcha>(ledger)),
      put_captcha_(std::make_unique<promotion::PutCaptcha>(ledger)),
      post_safetynet_(std::make_unique<promotion::PostSafetynet>(ledger)),
      put_safetynet_(std::make_unique<promotion::PutSafetynet>(ledger)),
      post_devicecheck_(std::make_unique<promotion::PostDevicecheck>(ledger)),
      put_devicecheck_(std::make_unique<promotion::PutDevicecheck>(ledger)),
      post_suggestions_(std::make_unique<promotion::PostSuggestions>(ledger)),
      post_suggestions_claim_(
          std::make_unique<promotion::PostSuggestionsClaim>(ledger)),
      post_claim_brave_(std::make_unique<promotion::PostClaimBrave>(ledger)),
      get_drain_(std::make_unique<promotion::GetDrain>(ledger)) {}

PromotionServer::~PromotionServer() = default;

promotion::GetAvailable* PromotionServer::get_available() const {
  return get_available_.get();
}

promotion::PostCreds* PromotionServer::post_creds() const {
  return post_creds_.get();
}

promotion::GetSignedCreds* PromotionServer::get_signed_creds() const {
  return get_signed_creds_.get();
}

promotion::PostClobberedClaims* PromotionServer::post_clobbered_claims() const {
  return post_clobbered_claims_.get();
}

promotion::PostBatLoss* PromotionServer::post_bat_loss() const {
  return post_bat_loss_.get();
}

promotion::PostWalletBrave* PromotionServer::post_wallet_brave() const {
  return post_wallet_brave_.get();
}

promotion::GetRecoverWallet* PromotionServer::get_recover_wallet() const {
  return get_recover_wallet_.get();
}

promotion::PostClaimBitflyer* PromotionServer::post_claim_bitflyer() const {
  return post_claim_bitflyer_.get();
}

promotion::PostClaimGemini* PromotionServer::post_claim_gemini() const {
  return post_claim_gemini_.get();
}

promotion::PostClaimUphold* PromotionServer::post_claim_uphold() const {
  return post_claim_uphold_.get();
}

promotion::GetWallet* PromotionServer::get_wallet() const {
  return get_wallet_.get();
}

promotion::PostCaptcha* PromotionServer::post_captcha() const {
  return post_captcha_.get();
}

promotion::GetCaptcha* PromotionServer::get_captcha() const {
  return get_captcha_.get();
}

promotion::PutCaptcha* PromotionServer::put_captcha() const {
  return put_captcha_.get();
}

promotion::PostSafetynet* PromotionServer::post_safetynet() const {
  return post_safetynet_.get();
}

promotion::PutSafetynet* PromotionServer::put_safetynet() const {
  return put_safetynet_.get();
}

promotion::PostDevicecheck* PromotionServer::post_devicecheck() const {
  return post_devicecheck_.get();
}

promotion::PutDevicecheck* PromotionServer::put_devicecheck() const {
  return put_devicecheck_.get();
}

promotion::PostSuggestions* PromotionServer::post_suggestions() const {
  return post_suggestions_.get();
}

promotion::PostSuggestionsClaim*
PromotionServer::post_suggestions_claim() const {
  return post_suggestions_claim_.get();
}

promotion::PostClaimBrave* PromotionServer::post_claim_brave() const {
  return post_claim_brave_.get();
}

promotion::GetDrain* PromotionServer::get_drain() const {
  return get_drain_.get();
}

promotion::DeleteClaim* PromotionServer::delete_claim() const {
  return delete_claim_.get();
}

}  // namespace endpoint
}  // namespace ledger

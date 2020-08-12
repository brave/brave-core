/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {
namespace endpoint {

PromotionServer::PromotionServer(bat_ledger::LedgerImpl* ledger):
    get_available_(new promotion::GetAvailable(ledger)),
    post_creds_(new promotion::PostCreds(ledger)),
    get_signed_creds_(new promotion::GetSignedCreds(ledger)),
    post_clobbered_claims_(new promotion::PostClobberedClaims(ledger)),
    post_bat_loss_(new promotion::PostBatLoss(ledger)),
    post_wallet_brave_(new promotion::PostWalletBrave(ledger)),
    get_recover_wallet_(new promotion::GetRecoverWallet(ledger)),
    post_claim_uphold_(new promotion::PostClaimUphold(ledger)),
    get_wallet_balance_(new promotion::GetWalletBalance(ledger)),
    post_captcha_(new promotion::PostCaptcha(ledger)),
    get_captcha_(new promotion::GetCaptcha(ledger)),
    put_captcha_(new promotion::PutCaptcha(ledger)),
    post_safetynet_(new promotion::PostSafetynet(ledger)),
    put_safetynet_(new promotion::PutSafetynet(ledger)),
    post_devicecheck_(new promotion::PostDevicecheck(ledger)),
    put_devicecheck_(new promotion::PutDevicecheck(ledger)),
    post_suggestions_(new promotion::PostSuggestions(ledger)),
    post_suggestions_claim_(new promotion::PostSuggestionsClaim(ledger)) {
}

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

promotion::PostClaimUphold* PromotionServer::post_claim_uphold() const {
  return post_claim_uphold_.get();
}

promotion::GetWalletBalance* PromotionServer::get_wallet_balance() const {
  return get_wallet_balance_.get();
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

}  // namespace endpoint
}  // namespace ledger

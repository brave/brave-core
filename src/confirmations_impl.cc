/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_impl.h"
#include "logging.h"
#include "static_values.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

using namespace std::placeholders;

namespace confirmations {

ConfirmationsImpl::ConfirmationsImpl(
    ConfirmationsClient* confirmations_client) :
    is_initialized_(false),
    is_wallet_initialized_(false),
    is_catalog_issuers_initialized_(false),
    refill_confirmations_timer_id_(0),
    refill_tokens_(std::make_unique<RefillTokens>(this, confirmations_client)),
    retry_getting_signed_tokens_timer_id_(0),
    unblinded_tokens_({}),
    redeem_token_(std::make_unique<RedeemToken>(this, confirmations_client)),
    unblinded_payment_tokens_({}),
    payout_confirmations_timer_id_(0),
    payout_tokens_(std::make_unique<PayoutTokens>(this, confirmations_client)),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing Confirmations";

  LoadState();
}

ConfirmationsImpl::~ConfirmationsImpl() {
  BLOG(INFO) << "Deinitializing Confirmations";

  StopRefillingConfirmations();
  StopRetryGettingSignedTokens();
  StopPayingOutConfirmations();
}

std::unique_ptr<base::ListValue> ConfirmationsImpl::Munge(
    const std::vector<std::string>& v) {
  base::ListValue* list = new base::ListValue();

  for (auto x : v) {
    list->AppendString(x);
  }

  return std::unique_ptr<base::ListValue>(list);
}

std::vector<std::string> ConfirmationsImpl::Unmunge(base::Value* value) {
  std::vector<std::string> v;

  base::ListValue list(value->GetList());

  for (size_t i = 0; i < list.GetSize(); i++) {
    base::Value* x;
    list.Get(i, &x);
    v.push_back(x->GetString());
  }

  return v;
}

void ConfirmationsImpl::UpdateConfirmationsIsReadyStatus() {
  bool is_ready = unblinded_tokens_.size() > 0 ? true : false;
  confirmations_client_->SetConfirmationsIsReady(is_ready);
}

std::vector<UnblindedToken> ConfirmationsImpl::GetUnblindedTokens() const {
  return unblinded_tokens_;
}

void ConfirmationsImpl::SetUnblindedTokens(
    const std::vector<UnblindedToken>& tokens) {
  unblinded_tokens_ = tokens;
  SaveState();
}

std::vector<UnblindedToken> ConfirmationsImpl::GetUnblindedPaymentTokens()
    const {
  return unblinded_payment_tokens_;
}

void ConfirmationsImpl::SetUnblindedPaymentTokens(
    const std::vector<UnblindedToken>& tokens) {
  unblinded_payment_tokens_ = tokens;
  SaveState();
}

std::string ConfirmationsImpl::ToJSON() {
  base::DictionaryValue dictionary;

  dictionary.SetKey("public_key", base::Value(public_key_));

  std::vector<std::string> catalog_issuers_names;
  std::vector<std::string> catalog_issuers_public_keys;
  for (const auto& issuer : catalog_issuers_) {
    catalog_issuers_names.push_back(issuer.second);
    catalog_issuers_public_keys.push_back(issuer.first);
  }

  dictionary.SetWithoutPathExpansion("catalog_issuers_names",
      Munge(catalog_issuers_names));

  dictionary.SetWithoutPathExpansion("catalog_issuers_public_keys",
      Munge(catalog_issuers_public_keys));

  std::vector<std::string> unblinded_tokens;
  for (auto& unblinded_token : unblinded_tokens_) {
    auto unblinded_token_base64 = unblinded_token.encode_base64();
    unblinded_tokens.push_back(unblinded_token_base64);
  }
  dictionary.SetWithoutPathExpansion("unblinded_tokens",
      Munge(unblinded_tokens));

  std::vector<std::string> unblinded_payment_tokens;
  for (auto& unblinded_payment_token : unblinded_payment_tokens_) {
    auto unblinded_payment_token_base64 =
        unblinded_payment_token.encode_base64();
    unblinded_payment_tokens.push_back(unblinded_payment_token_base64);
  }
  dictionary.SetWithoutPathExpansion("unblinded_payment_tokens",
      Munge(unblinded_payment_tokens));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool ConfirmationsImpl::FromJSON(const std::string& json) {
  std::unique_ptr<base::Value> value(base::JSONReader::Read(json));
  if (!value) {
    return false;
  }

  base::DictionaryValue* dictionary;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  base::Value *v;

  if (!(v = dictionary->FindKey("public_key"))) {
    return false;
  }
  public_key_ = v->GetString();

  if (!(v = dictionary->FindKey("catalog_issuers_names"))) {
    return false;
  }
  auto catalog_issuers_names = Unmunge(v);

  if (!(v = dictionary->FindKey("catalog_issuers_public_keys"))) {
    return false;
  }
  auto catalog_issuers_public_keys = Unmunge(v);

  catalog_issuers_.clear();
  for (size_t i = 0; i < catalog_issuers_names.size(); i++) {
    auto name = catalog_issuers_names.at(i);
    auto public_key = catalog_issuers_public_keys.at(i);

    catalog_issuers_.insert({public_key, name});
  }

  if (!(v = dictionary->FindKey("unblinded_tokens"))) {
    return false;
  }
  auto unblinded_tokens_base64 = Unmunge(v);
  std::vector<UnblindedToken> unblinded_tokens;
  for (auto& unblinded_token_base64 : unblinded_tokens_base64) {
    auto unblinded_token =
        UnblindedToken::decode_base64(unblinded_token_base64);
    unblinded_tokens.push_back(unblinded_token);
  }
  unblinded_tokens_ = unblinded_tokens;

  if (!(v = dictionary->FindKey("unblinded_payment_tokens"))) {
    return false;
  }
  auto unblinded_payment_tokens_base64 = Unmunge(v);
  std::vector<UnblindedToken> unblinded_payment_tokens;
  for (auto& unblinded_payment_token_base64 :
      unblinded_payment_tokens_base64) {
    auto unblinded_payment_token =
        UnblindedToken::decode_base64(unblinded_payment_token_base64);
    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }
  unblinded_payment_tokens_ = unblinded_payment_tokens;

  return true;
}

void ConfirmationsImpl::SaveState() {
  BLOG(INFO) << "Saving confirmations state";

  std::string json = ToJSON();
  auto callback = std::bind(&ConfirmationsImpl::OnStateSaved, this, _1);
  confirmations_client_->Save(_confirmations_name, json, callback);

  UpdateConfirmationsIsReadyStatus();
}

void ConfirmationsImpl::OnStateSaved(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to save confirmations state";

    return;
  }

  BLOG(INFO) << "Successfully saved confirmations state";
}

void ConfirmationsImpl::LoadState() {
  BLOG(INFO) << "Loading confirmations state";

  auto callback = std::bind(&ConfirmationsImpl::OnStateLoaded, this, _1, _2);
  confirmations_client_->Load(_confirmations_name, callback);
}

void ConfirmationsImpl::OnStateLoaded(
    const Result result,
    const std::string& json) {
  auto confirmations_json = json;

  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to load confirmations state, resetting to default"
        << " values";

    confirmations_json = ToJSON();
  }

  if (!FromJSON(confirmations_json)) {
    BLOG(ERROR) << "Failed to parse confirmations state: "
        << confirmations_json;
    return;
  }

  BLOG(INFO) << "Successfully loaded confirmations state";

  UpdateConfirmationsIsReadyStatus();

  RefillConfirmations();
  PayoutConfirmations();
}

void ConfirmationsImpl::ResetState() {
  BLOG(INFO) << "Resetting confirmations to default state";

  auto callback = std::bind(&ConfirmationsImpl::OnStateReset, this, _1);
  confirmations_client_->Reset(_confirmations_name, callback);
}

void ConfirmationsImpl::OnStateReset(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to reset confirmations state";

    return;
  }

  BLOG(INFO) << "Successfully reset confirmations state";
}

void ConfirmationsImpl::SetWalletInfo(std::unique_ptr<WalletInfo> info) {
  wallet_info_.payment_id = info->payment_id;
  wallet_info_.signing_key = info->signing_key;

  BLOG(INFO) << "SetWalletInfo:";
  BLOG(INFO) << "  Payment Id: " << wallet_info_.payment_id;
  BLOG(INFO) << "  Signing key: " << wallet_info_.signing_key;

  is_wallet_initialized_ = true;

  if (is_catalog_issuers_initialized_ && !is_initialized_) {
    is_initialized_ = true;
    BLOG(INFO) << "Successfully initialized";
  }
}

void ConfirmationsImpl::SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) {
  BLOG(INFO) << "SetCatalogIssuers:";
  BLOG(INFO) << "  Public key: " << info->public_key;
  BLOG(INFO) << "  Issuers:";

  for (const auto& issuer : info->issuers) {
    BLOG(INFO) << "    Name: " << issuer.name;
    BLOG(INFO) << "    Public key: " << issuer.public_key;
  }

  public_key_ = info->public_key;

  catalog_issuers_.clear();
  for (const auto& issuer : info->issuers) {
    catalog_issuers_.insert({issuer.public_key, issuer.name});
  }

  SaveState();

  is_catalog_issuers_initialized_ = true;

  if (is_wallet_initialized_ && !is_initialized_) {
    is_initialized_ = true;
    BLOG(INFO) << "Successfully initialized";
  }
}

std::map<std::string, std::string> ConfirmationsImpl::GetCatalogIssuers()
    const {
  return catalog_issuers_;
}

void ConfirmationsImpl::AdSustained(std::unique_ptr<NotificationInfo> info) {
  BLOG(INFO) << "AdSustained:";
  BLOG(INFO) << "  creativeSetId: " << info->creative_set_id;
  BLOG(INFO) << "  category: " << info->category;
  BLOG(INFO) << "  notificationUrl: " << info->url;
  BLOG(INFO) << "  notificationText: " << info->text;
  BLOG(INFO) << "  advertiser: " << info->advertiser;
  BLOG(INFO) << "  uuid: " << info->uuid;

  redeem_token_->Redeem(info->uuid, public_key_);
}

void ConfirmationsImpl::OnTimer(const uint32_t timer_id) {
  BLOG(INFO) << "OnTimer:" << std::endl
      << "  timer_id: " << std::to_string(timer_id) << std::endl
      << "  refill_confirmations_timer_id_: "
      << std::to_string(refill_confirmations_timer_id_) << std::endl
      << "  retry_getting_signed_tokens_timer_id_: "
      << std::to_string(retry_getting_signed_tokens_timer_id_) << std::endl
      << "  payout_confirmations_timer_id_: "
      << std::to_string(payout_confirmations_timer_id_);

  if (timer_id == refill_confirmations_timer_id_) {
    RefillConfirmations();
  } else if (timer_id == retry_getting_signed_tokens_timer_id_) {
    RetryGettingSignedTokens();
  } else if (timer_id == payout_confirmations_timer_id_) {
    PayoutConfirmations();
  } else {
    LOG(WARNING) << "Unexpected OnTimer: " << std::to_string(timer_id);
  }
}

void ConfirmationsImpl::StartRefillingConfirmations(
    const uint64_t start_timer_in) {
  StopRefillingConfirmations();

  refill_confirmations_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (refill_confirmations_timer_id_ == 0) {
    BLOG(ERROR)
        << "Failed to start refilling confirmations due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start refilling confirmations in " << start_timer_in
  << " seconds";
}

void ConfirmationsImpl::RefillConfirmations() {
  BLOG(INFO) << "Refill confirmations";

  if (!is_initialized_) {
    BLOG(INFO) << "Failed to refill confirmations as not initialized";
    StartRefillingConfirmations(kOneMinuteInSeconds);
    return;
  }

  refill_tokens_->Refill(wallet_info_, public_key_);
}

void ConfirmationsImpl::StopRefillingConfirmations() {
  if (!IsRefillingConfirmations()) {
    return;
  }

  BLOG(INFO) << "Stopped refilling confirmations";

  confirmations_client_->KillTimer(refill_confirmations_timer_id_);
  refill_confirmations_timer_id_ = 0;
}

bool ConfirmationsImpl::IsRefillingConfirmations() const {
  if (refill_confirmations_timer_id_ == 0) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::StartPayingOutConfirmations(
    const uint64_t start_timer_in) {
  StopPayingOutConfirmations();

  payout_confirmations_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (payout_confirmations_timer_id_ == 0) {
    BLOG(ERROR)
        << "Failed to start paying out confirmations due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start paying out confirmations in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::PayoutConfirmations() {
  BLOG(INFO) << "Payout confirmations";

  if (!is_initialized_) {
    BLOG(INFO) << "Failed to payout confirmations as not initialized";
    StartPayingOutConfirmations(kOneMinuteInSeconds);
    return;
  }

  payout_tokens_->Payout(wallet_info_, public_key_);
}

void ConfirmationsImpl::StopPayingOutConfirmations() {
  if (!IsPayingOutConfirmations()) {
    return;
  }

  BLOG(INFO) << "Stopped paying out confirmations";

  confirmations_client_->KillTimer(payout_confirmations_timer_id_);
  payout_confirmations_timer_id_ = 0;
}

bool ConfirmationsImpl::IsPayingOutConfirmations() const {
  if (payout_confirmations_timer_id_ == 0) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::StartRetryGettingSignedTokens(
    const uint64_t start_timer_in) {
  StopRetryGettingSignedTokens();

  retry_getting_signed_tokens_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (retry_getting_signed_tokens_timer_id_ == 0) {
    BLOG(ERROR)
        << "Failed to start getting signed tokens due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start getting signed tokens in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::RetryGettingSignedTokens() {
  BLOG(INFO) << "Retry getting signed tokens";

  if (!is_initialized_) {
    BLOG(INFO) << "Failed to retry getting signed tokens as not initialized";
    StartRetryGettingSignedTokens(kOneMinuteInSeconds);
    return;
  }

  refill_tokens_->RetryGettingSignedTokens();
}

void ConfirmationsImpl::StopRetryGettingSignedTokens() {
  if (!IsRetryingToGetSignedTokens()) {
    return;
  }

  BLOG(INFO) << "Stopped getting signed tokens";

  confirmations_client_->KillTimer(retry_getting_signed_tokens_timer_id_);
  retry_getting_signed_tokens_timer_id_ = 0;
}

bool ConfirmationsImpl::IsRetryingToGetSignedTokens() const {
  if (retry_getting_signed_tokens_timer_id_ == 0) {
    return false;
  }

  return true;
}

}  // namespace confirmations

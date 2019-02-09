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
    unblinded_tokens_(std::make_unique<UnblindedTokens>(this)),
    unblinded_payment_tokens_(std::make_unique<UnblindedTokens>(this)),
    retry_getting_signed_tokens_timer_id_(0),
    refill_tokens_(std::make_unique<RefillTokens>(
        this, confirmations_client, unblinded_tokens_.get())),
    redeem_token_(std::make_unique<RedeemToken>(this, confirmations_client,
        unblinded_tokens_.get(), unblinded_payment_tokens_.get())),
    payout_redeemed_tokens_timer_id_(0),
    payout_tokens_(std::make_unique<PayoutTokens>(this, confirmations_client,
        unblinded_payment_tokens_.get())),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing Confirmations";

  LoadState();
}

ConfirmationsImpl::~ConfirmationsImpl() {
  BLOG(INFO) << "Deinitializing Confirmations";

  StopRetryingToGetRefillSignedTokens();
  StopPayingOutRedeemedTokens();
}

void ConfirmationsImpl::CheckReady() {
  if (is_initialized_) {
    return;
  }

  if (!is_wallet_initialized_ || !is_catalog_issuers_initialized_) {
    return;
  }

  is_initialized_ = true;
  BLOG(INFO) << "Successfully initialized";

  PayoutRedeemedTokens();
  RefillTokensIfNecessary();
}

void ConfirmationsImpl::NotifyAdsIfConfirmationsIsReady() {
  bool is_ready = unblinded_tokens_->IsEmpty() ? false : true;
  confirmations_client_->SetConfirmationsIsReady(is_ready);
}

// TODO(Terry Mancey): Refactor for unit tests
std::unique_ptr<base::ListValue> ConfirmationsImpl::Munge(
    const std::vector<std::string>& v) {
  base::ListValue* list = new base::ListValue();

  for (auto x : v) {
    list->AppendString(x);
  }

  return std::unique_ptr<base::ListValue>(list);
}

// TODO(Terry Mancey): Refactor for unit tests
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

// TODO(Terry Mancey): Refactor for unit tests
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

  auto unblinded_tokens_base64 = unblinded_tokens_->ToBase64();
  dictionary.SetWithoutPathExpansion("unblinded_tokens",
      Munge(unblinded_tokens_base64));

  auto unblinded_payment_tokens_base64 =
      unblinded_payment_tokens_->ToBase64();
  dictionary.SetWithoutPathExpansion("unblinded_payment_tokens",
      Munge(unblinded_payment_tokens_base64));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

// TODO(Terry Mancey): Refactor for unit tests
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
  unblinded_tokens_->FromBase64(unblinded_tokens_base64);

  if (!(v = dictionary->FindKey("unblinded_payment_tokens"))) {
    return false;
  }
  auto unblinded_payment_tokens_base64 = Unmunge(v);
  unblinded_payment_tokens_->FromBase64(unblinded_payment_tokens_base64);

  return true;
}

void ConfirmationsImpl::SaveState() {
  BLOG(INFO) << "Saving confirmations state";

  std::string json = ToJSON();
  auto callback = std::bind(&ConfirmationsImpl::OnStateSaved, this, _1);
  confirmations_client_->Save(_confirmations_name, json, callback);

  NotifyAdsIfConfirmationsIsReady();
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

  NotifyAdsIfConfirmationsIsReady();

  CheckReady();
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
  wallet_info_ = WalletInfo(*info);
  BLOG(INFO) << "SetWalletInfo:";
  BLOG(INFO) << "  Payment id: " << wallet_info_.payment_id;
  BLOG(INFO) << "  Public key: " << wallet_info_.public_key;

  is_wallet_initialized_ = true;

  CheckReady();
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

  CheckReady();
}

std::map<std::string, std::string> ConfirmationsImpl::GetCatalogIssuers()
    const {
  return catalog_issuers_;
}

bool ConfirmationsImpl::IsValidPublicKeyForCatalogIssues(
    const std::string& public_key) {
  auto it = catalog_issuers_.find(public_key);
  if (it == catalog_issuers_.end()) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::AdSustained(std::unique_ptr<NotificationInfo> info) {
  BLOG(INFO) << "AdSustained:";
  BLOG(INFO) << "  creativeSetId: " << info->creative_set_id;
  BLOG(INFO) << "  category: " << info->category;
  BLOG(INFO) << "  notificationUrl: " << info->url;
  BLOG(INFO) << "  notificationText: " << info->text;
  BLOG(INFO) << "  advertiser: " << info->advertiser;
  BLOG(INFO) << "  uuid: " << info->uuid;

  redeem_token_->Redeem(info->uuid);
}

void ConfirmationsImpl::OnTimer(const uint32_t timer_id) {
  BLOG(INFO) << "OnTimer:" << std::endl
      << "  timer_id: " << std::to_string(timer_id) << std::endl
      << "  retry_getting_signed_tokens_timer_id_: "
      << std::to_string(retry_getting_signed_tokens_timer_id_) << std::endl
      << "  payout_redeemed_tokens_timer_id_: "
      << std::to_string(payout_redeemed_tokens_timer_id_);

  if (timer_id == retry_getting_signed_tokens_timer_id_) {
    RetryGettingRefillSignedTokens();
  } else if (timer_id == payout_redeemed_tokens_timer_id_) {
    PayoutRedeemedTokens();
  } else {
    BLOG(WARNING) << "Unexpected OnTimer with timer_id: " << timer_id;
  }
}

void ConfirmationsImpl::RefillTokensIfNecessary() {
  refill_tokens_->Refill(wallet_info_, public_key_);
}

void ConfirmationsImpl::StartPayingOutRedeemedTokens(
    const uint64_t start_timer_in) {
  StopPayingOutRedeemedTokens();

  payout_redeemed_tokens_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (payout_redeemed_tokens_timer_id_ == 0) {
    BLOG(ERROR)
        << "Failed to start paying out redeemed tokens due to an invalid timer";
    return;
  }

  BLOG(INFO) << "Start paying out redeemed tokens in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::PayoutRedeemedTokens() {
  payout_tokens_->Payout(wallet_info_);
}

void ConfirmationsImpl::StopPayingOutRedeemedTokens() {
  if (!IsPayingOutRedeemedTokens()) {
    return;
  }

  BLOG(INFO) << "Stopped paying out redeemed tokens";

  confirmations_client_->KillTimer(payout_redeemed_tokens_timer_id_);
  payout_redeemed_tokens_timer_id_ = 0;
}

bool ConfirmationsImpl::IsPayingOutRedeemedTokens() const {
  if (payout_redeemed_tokens_timer_id_ == 0) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::StartRetryingToGetRefillSignedTokens(
    const uint64_t start_timer_in) {
  StopRetryingToGetRefillSignedTokens();

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

void ConfirmationsImpl::RetryGettingRefillSignedTokens() {
  refill_tokens_->RetryGettingSignedTokens();
}

void ConfirmationsImpl::StopRetryingToGetRefillSignedTokens() {
  if (!IsRetryingToGetRefillSignedTokens()) {
    return;
  }

  BLOG(INFO) << "Stopped getting signed tokens";

  confirmations_client_->KillTimer(retry_getting_signed_tokens_timer_id_);
  retry_getting_signed_tokens_timer_id_ = 0;
}

bool ConfirmationsImpl::IsRetryingToGetRefillSignedTokens() const {
  if (retry_getting_signed_tokens_timer_id_ == 0) {
    return false;
  }

  return true;
}

}  // namespace confirmations

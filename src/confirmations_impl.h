/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_
#define BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_

#include <string>
#include <mutex>

#include "brave/vendor/challenge_bypass_ristretto_ffi/src/wrapper.hpp"
#include "base/values.h"
#include "bat/confirmations/confirmations.h"
#include "bat/confirmations/notification_info.h"
#include "bat/confirmations/issuers_info.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"

#define CONFIRMATIONS_SIGNATURE_ALGORITHM "ed25519"

namespace confirmations {

using namespace challenge_bypass_ristretto;

class Semaphore {
 public:
  explicit Semaphore(int count = 0) : count_(count) {}

  void signal() {
    std::unique_lock<std::mutex> lock(mutex_);
    count_++;
    condition_variable_.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (count_ == 0) {
      condition_variable_.wait(lock);
    }

    count_--;
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_variable_;
  int count_;
};

class ConfirmationsImpl : public Confirmations,
                          public net::URLFetcherDelegate {
 public:
  explicit ConfirmationsImpl(ConfirmationsClient* confirmations_client);
  ~ConfirmationsImpl() override;

  void SetWalletInfo(std::unique_ptr<WalletInfo> info) override;
  void SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) override;
  bool IsReadyToShowAds();
  void AdSustained(std::unique_ptr<NotificationInfo> info) override;
  void OnTimer(const uint32_t timer_id) override;

 private:
  bool is_initialized_;

  const size_t low_token_threshold = 20;
  const size_t refill_amount = 5 * low_token_threshold;

  WalletInfo wallet_info_;

  Semaphore semaphore_;
  std::string response_;
  int response_code_;

  void URLFetchSync(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      net::URLFetcher::RequestType request_type);
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  uint32_t step_2_refill_confirmations_timer_id_;
  void StartRefillingConfirmations(const uint64_t start_timer_in);
  void RefillConfirmations();
  void StopRefillingConfirmations();
  bool IsRefillingConfirmations() const;

  uint32_t step_4_retrieve_payment_ious_timer_id_;
  void StartRetrievingPaymentIOUS(const uint64_t start_timer_in);
  void RetrievePaymentIOUS();
  void StopRetrievingPaymentIOUS();
  bool IsRetrievingPaymentIOUS() const;

  uint32_t step_5_cash_in_payment_ious_timer_id_;
  void StartCashingInPaymentIOUS(const uint64_t start_timer_in);
  void CashInPaymentIOUS();
  void StopCashingInPaymentIOUS();
  bool IsCashingInPaymentIOUS() const;

  /////////////////////////////////////////////////////////////////////////////
  // persist these properties

  // If unset or "0", assume we haven't gotten one
  std::string issuers_version_ = "0";

  // If this changes what we can do is burn .*confirmation_tokens.* & repop
  std::string server_confirmation_key_;

  std::vector<std::string> server_bat_payment_names;
  std::vector<std::string> server_bat_payment_keys;

  std::vector<std::string> original_confirmation_tokens;
  std::vector<std::string> blinded_confirmation_tokens;
  std::vector<std::string> signed_blinded_confirmation_tokens;

  std::vector<std::string> payment_token_json_bundles;
  std::vector<std::string> signed_blinded_payment_token_json_bundles;
  std::vector<std::string> fully_submitted_payment_bundles;

  /////////////////////////////////////////////////////////////////////////////

  std::string GetServerUrl();
  int GetServerPort();

  void Step1StoreTheServersConfirmationsPublicKeyAndGenerator(
      std::string confirmations_GH_pair,
      std::vector<std::string> bat_names,
      std::vector<std::string> bat_keys);
  void Step2RefillConfirmationsIfNecessary(
      std::string real_wallet_address,
      std::string real_wallet_address_secret_key,
      std::string server_confirmation_key);
  void Step3RedeemConfirmation(std::string real_creative_instance_id);
  void Step4RetrievePaymentIOUs();
  void Step5CashInPaymentIOUs(std::string real_wallet_address);

  bool VerifyBatchDLEQProof(
      std::string proof_string,
      std::vector<std::string> blind_strings,
      std::vector<std::string> signed_strings,
      std::string public_key_string);

  void PopFrontConfirmation();
  void PopFrontPayment();

  void SaveState();
  void OnStateSaved(const Result result);
  void LoadState();
  void OnStateLoaded(const Result result, const std::string& json);
  void ResetState();
  void OnStateReset(const Result result);

  std::string toJSONString();
  bool FromJSONString(std::string json);

  void VectorConcat(
      std::vector<std::string>* dest,
      std::vector<std::string>* source);
  std::unique_ptr<base::ListValue> Munge(std::vector<std::string> v);
  std::vector<std::string> Unmunge(base::Value *value);
  std::string BATNameFromBATPublicKey(std::string token);
  bool ProcessIOUBundle(std::string bundle_json);

  // convert std::string of ascii-hex to raw data vector<uint8_t>
  std::vector<uint8_t> RawDataBytesVectorFromASCIIHexString(std::string ascii);

  // these functions are copy-pasta from the ledger library and in the future
  // should be refactored somehow
  std::string Sign(
      std::string* keys,
      std::string* values,
      const unsigned int& size,
      const std::string& keyId,
      const std::vector<uint8_t>& secretKey);
  std::vector<uint8_t> GetSHA256(const std::string& in);
  std::string GetBase64(const std::vector<uint8_t>& in);

  ConfirmationsClient* confirmations_client_;  // NOT OWNED

  // Not copyable, not assignable
  ConfirmationsImpl(const ConfirmationsImpl&) = delete;
  ConfirmationsImpl& operator=(const ConfirmationsImpl&) = delete;
};

class MockServer {
 public:
  MockServer();
  ~MockServer();

  SigningKey signing_key = SigningKey::random();
  PublicKey public_key = signing_key.public_key();

  std::vector<std::string> signed_tokens;
  std::string batch_dleq_proof;

  void GenerateSignedBlindedTokensAndProof(
      std::vector<std::string> blinded_tokens);

  void Test();
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_

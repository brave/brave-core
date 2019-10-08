/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_HELPER_H_
#define BRAVELEDGER_BAT_HELPER_H_

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/static_values.h"

namespace braveledger_bat_helper {
bool isProbiValid(const std::string& number);

struct UNSIGNED_TX {
  UNSIGNED_TX();
  ~UNSIGNED_TX();

  std::string amount_;
  std::string currency_;
  std::string destination_;
};

struct RECONCILE_PAYLOAD_ST {
  RECONCILE_PAYLOAD_ST();
  ~RECONCILE_PAYLOAD_ST();

  std::string requestType_;
  std::string request_signedtx_headers_digest_;
  std::string request_signedtx_headers_signature_;
  UNSIGNED_TX request_signedtx_body_;
  std::string request_signedtx_octets_;
  std::string request_viewingId_;
  std::string request_surveyorId_;
};

struct WALLET_INFO_ST {
  WALLET_INFO_ST();
  WALLET_INFO_ST(const WALLET_INFO_ST&);
  ~WALLET_INFO_ST();

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string paymentId_;
  std::string addressBAT_;
  std::string addressBTC_;
  std::string addressCARD_ID_;
  std::string addressETH_;
  std::string addressLTC_;
  std::vector<uint8_t> keyInfoSeed_;
};

struct TRANSACTION_BALLOT_ST {
  TRANSACTION_BALLOT_ST();
  ~TRANSACTION_BALLOT_ST();

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string publisher_;
  unsigned int offset_ = 0u;
};

struct TRANSACTION_ST {
  TRANSACTION_ST();
  TRANSACTION_ST(const TRANSACTION_ST& transaction);
  ~TRANSACTION_ST();

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string viewingId_;
  std::string surveyorId_;
  std::map<std::string, double> contribution_rates_;
  std::string contribution_probi_;
  std::string submissionStamp_;
  std::string anonizeViewingId_;
  std::string registrarVK_;
  std::string masterUserToken_;
  std::vector<std::string> surveyorIds_;
  unsigned int votes_ = 0u;
  std::vector<TRANSACTION_BALLOT_ST> ballots_;
};

struct BALLOT_ST {
  BALLOT_ST();
  BALLOT_ST(const BALLOT_ST& ballot);
  ~BALLOT_ST();

  // Load from json string
  bool loadFromJson(const std::string & json);

  std::string viewingId_;
  std::string surveyorId_;
  std::string publisher_;
  unsigned int offset_ = 0u;
  std::string prepareBallot_;
  std::string proofBallot_;
  uint64_t delayStamp_ = 0u;
};

struct BATCH_VOTES_INFO_ST {
  BATCH_VOTES_INFO_ST();
  BATCH_VOTES_INFO_ST(const BATCH_VOTES_INFO_ST&);
  ~BATCH_VOTES_INFO_ST();

  // Load from json string
  bool loadFromJson(const std::string & json);

  std::string surveyorId_;
  std::string proof_;
};

struct BATCH_VOTES_ST {
  BATCH_VOTES_ST();
  BATCH_VOTES_ST(const BATCH_VOTES_ST&);
  ~BATCH_VOTES_ST();

  // Load from json string
  bool loadFromJson(const std::string & json);

  std::string publisher_;
  std::vector<BATCH_VOTES_INFO_ST> batchVotesInfo_;
};

struct GRANT {
  GRANT();
  GRANT(const GRANT& properties);
  ~GRANT();

  // load from json string
  bool loadFromJson(const std::string & json);
  std::string altcurrency;
  std::string probi;
  uint64_t expiryTime;
  std::string promotionId;
  std::string type;
};

struct GRANT_RESPONSE {
  GRANT_RESPONSE();
  GRANT_RESPONSE(const GRANT_RESPONSE& properties);
  ~GRANT_RESPONSE();

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string promotionId;
  uint64_t minimumReconcileTimestamp;
  uint64_t protocolVersion;
  std::string type;
};

struct WALLET_PROPERTIES_ST {
  WALLET_PROPERTIES_ST();
  ~WALLET_PROPERTIES_ST();
  WALLET_PROPERTIES_ST(const WALLET_PROPERTIES_ST& properties);

  // load from json string
  bool loadFromJson(const std::string & json);

  double fee_amount_;
  std::vector<double> parameters_choices_;
  std::vector<GRANT> grants_;
};

struct REPORT_BALANCE_ST {
  REPORT_BALANCE_ST();
  REPORT_BALANCE_ST(const REPORT_BALANCE_ST&);
  ~REPORT_BALANCE_ST();

  bool loadFromJson(const std::string &json);

  std::string opening_balance_ = "0";
  std::string closing_balance_ = "0";
  std::string deposits_ = "0";
  std::string grants_ = "0";
  std::string earning_from_ads_ = "0";
  std::string auto_contribute_ = "0";
  std::string recurring_donation_ = "0";
  std::string one_time_donation_ = "0";
  std::string total_ = "0";
};

struct PUBLISHER_STATE_ST {
  PUBLISHER_STATE_ST();
  PUBLISHER_STATE_ST(const PUBLISHER_STATE_ST&);
  ~PUBLISHER_STATE_ST();

  // load from json string
  bool loadFromJson(const std::string &json);

  uint64_t min_publisher_duration_ =
      braveledger_ledger::_default_min_publisher_duration;  // In seconds
  unsigned int min_visits_ = 1u;
  bool allow_non_verified_ = true;
  bool allow_videos_ = true;
  std::map<std::string, REPORT_BALANCE_ST> monthly_balances_;
  bool migrate_score_2 = false;
  std::vector<std::string> processed_pending_publishers;
};

struct PUBLISHER_ST {
  PUBLISHER_ST();
  ~PUBLISHER_ST();
  bool operator<(const PUBLISHER_ST& rhs) const;

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string id_;
  uint64_t duration_ = 0u;
  double score_ = .0;
  unsigned int visits_ = 0;

  // The mathematically rounded publisher voting weight, as described
  // below.
  unsigned int percent_ = 0;

  // The exact weight to use when calculating the number of votes to
  // cast for each publisher.
  double weight_ = .0;

  unsigned int status_ = 0;
};

struct WINNERS_ST {
  WINNERS_ST();
  ~WINNERS_ST();

  PUBLISHER_ST publisher_data_;
  unsigned int votes_ = 0;
};

typedef std::vector<braveledger_bat_helper::WINNERS_ST> Winners;

struct SURVEYOR_INFO_ST {
  SURVEYOR_INFO_ST();
  ~SURVEYOR_INFO_ST();

  std::string surveyorId_;
};

struct SURVEYOR_ST {
  SURVEYOR_ST();
  ~SURVEYOR_ST();

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string signature_;
  std::string surveyorId_;
  std::string surveyVK_;
  std::string registrarVK_;
  std::string surveySK_;
};

struct RECONCILE_DIRECTION {
  RECONCILE_DIRECTION();
  RECONCILE_DIRECTION(const std::string& publisher_key,
                      int amount,
                      const std::string& currency);
  ~RECONCILE_DIRECTION();

  bool loadFromJson(const std::string &json);

  std::string publisher_key_;
  int amount_;
  std::string currency_;
};

typedef std::vector<RECONCILE_DIRECTION> Directions;
typedef std::vector<PUBLISHER_ST> PublisherList;

struct CURRENT_RECONCILE {
  CURRENT_RECONCILE();
  CURRENT_RECONCILE(const CURRENT_RECONCILE&);
  ~CURRENT_RECONCILE();

  // load from json string
  bool loadFromJson(const std::string & json);

  std::string viewingId_;
  std::string anonizeViewingId_;
  std::string registrarVK_;
  std::string preFlight_;
  std::string masterUserToken_;
  SURVEYOR_INFO_ST surveyorInfo_;
  uint64_t timestamp_ = 0u;
  std::map<std::string, double> rates_;
  std::string amount_;
  std::string currency_;
  double fee_;
  Directions directions_;
  ledger::RewardsCategory category_;
  PublisherList list_;
  ledger::ContributionRetry retry_step_;
  int retry_level_;
  std::string destination_;
  std::string proof_;
};

typedef std::vector<GRANT> Grants;
typedef std::vector<TRANSACTION_ST> Transactions;
typedef std::vector<BALLOT_ST> Ballots;
typedef std::vector<BATCH_VOTES_ST> BatchVotes;
typedef std::map<std::string, CURRENT_RECONCILE> CurrentReconciles;

struct CLIENT_STATE_ST {
  CLIENT_STATE_ST();
  CLIENT_STATE_ST(const CLIENT_STATE_ST&);
  ~CLIENT_STATE_ST();

  // Load from json string
  bool loadFromJson(const std::string & json);

  WALLET_INFO_ST walletInfo_;
  WALLET_PROPERTIES_ST walletProperties_;
  uint64_t bootStamp_ = 0u;
  uint64_t reconcileStamp_ = 0u;
  uint64_t last_grant_fetch_stamp_ = 0u;
  std::string personaId_;
  std::string userId_;
  std::string registrarVK_;
  std::string masterUserToken_;
  std::string preFlight_;
  std::string fee_currency_;
  std::string settings_ = AD_FREE_SETTINGS;
  double fee_amount_ = .0;
  bool user_changed_fee_ = false;
  unsigned int days_ = 0u;
  Transactions transactions_;
  Ballots ballots_;
  std::string ruleset_;
  std::string rulesetV2_;
  BatchVotes batch_;
  Grants grants_;
  CurrentReconciles current_reconciles_;
  bool auto_contribute_ = false;
  bool rewards_enabled_ = false;
  std::map<std::string, bool> inline_tip_;
};

struct GRANTS_PROPERTIES_ST {
  GRANTS_PROPERTIES_ST();
  ~GRANTS_PROPERTIES_ST();
  GRANTS_PROPERTIES_ST(const GRANTS_PROPERTIES_ST& properties);

  //  load from json string
  bool loadFromJson(const std::string & json);
  std::vector<GRANT_RESPONSE> grants_;
};

struct BATCH_PROOF {
  BATCH_PROOF();
  ~BATCH_PROOF();

  TRANSACTION_ST transaction_;
  BALLOT_ST ballot_;
};

typedef std::vector<braveledger_bat_helper::BATCH_PROOF> BatchProofs;

enum class SERVER_TYPES {
  LEDGER,
  BALANCE,
  PUBLISHER,
  PUBLISHER_DISTRO
};

using SaveVisitSignature = void(const std::string&, uint64_t);
using SaveVisitCallback = std::function<SaveVisitSignature>;

bool getJSONValue(const std::string& fieldName,
                  const std::string& json,
                  std::string* value);

bool getJSONList(const std::string& fieldName,
                 const std::string& json,
                 std::vector<std::string>* value);

bool getJSONWalletInfo(const std::string& json,
                       WALLET_INFO_ST* walletInfo,
                       std::string* fee_currency,
                       double* fee_amount,
                       unsigned int* days);

bool getJSONUnsignedTx(const std::string& json, UNSIGNED_TX* unsignedTx);

bool getJSONTransaction(const std::string& json, TRANSACTION_ST* transaction);

bool getJSONRates(const std::string& json,
                  std::map<std::string, double>* rates);

bool getJSONTwitchProperties(
    const std::string& json,
    std::vector<std::map<std::string, std::string>>* parts);

bool getJSONBatchSurveyors(const std::string& json,
                           std::vector<std::string>* surveyors);

bool getJSONRecoverWallet(const std::string& json,
                          double* balance,
                          std::vector<GRANT>* grants);

bool getJSONResponse(const std::string& json,
                     unsigned int* statusCode,
                     std::string* error);

bool getJSONAddresses(const std::string& json,
                      std::map<std::string, std::string>* addresses);

bool getJSONMessage(const std::string& json,
                     std::string* message);

std::vector<uint8_t> generateSeed();

std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed);

bool getPublicKeyFromSeed(const std::vector<uint8_t>& seed,
                          std::vector<uint8_t>* publicKey,
                          std::vector<uint8_t>* secretKey);

std::string uint8ToHex(const std::vector<uint8_t>& in);

std::string stringify(std::string* keys,
                      std::string* values,
                      const unsigned int size);

std::string stringifyReconcilePayloadSt(
    const RECONCILE_PAYLOAD_ST& reconcile_payload);

std::string stringifyUnsignedTx(const UNSIGNED_TX& unsignedTx);

std::string stringifyBatch(std::vector<BATCH_VOTES_INFO_ST> payload);

std::vector<uint8_t> getSHA256(const std::string& in);

std::string getBase64(const std::vector<uint8_t>& in);

bool getFromBase64(const std::string& in, std::vector<uint8_t>* out);

// Sign using ed25519 algorithm
std::string sign(std::string* keys,
                 std::string* values,
                 const unsigned int size,
                 const std::string& keyId,
                 const std::vector<uint8_t>& secretKey);

uint64_t currentTime();

std::string buildURL(const std::string& path,
                     const std::string& prefix = "",
                     const SERVER_TYPES& server = SERVER_TYPES::LEDGER);

std::vector<std::string> split(const std::string& s, char delim);

bool ignore_for_testing();

void set_ignore_for_testing(bool ignore);

uint8_t niceware_mnemonic_to_bytes(
    const std::string& w,
    std::vector<uint8_t>* bytes_out,
    size_t* written,
    std::vector<std::string> wordDictionary);

bool HasSameDomainAndPath(
    const std::string& url,
    const std::string& to_match,
    const std::string& path);

}  // namespace braveledger_bat_helper

#endif  // BRAVELEDGER_BAT_HELPER_H_

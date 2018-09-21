/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_helper.h"

#include <sstream>
#include <random>
#include <utility>
#include <iomanip>
#include <ctime>
#include <memory>
#include <iostream>
#include <string>

#include <openssl/base64.h>
#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>

#include "bat/ledger/ledger.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"
#include "tweetnacl.h"

//#include "crypto/hkdf.h"

namespace braveledger_bat_helper {

namespace {
static bool ignore_ = false;
}  // namespace

  REQUEST_CREDENTIALS_ST::REQUEST_CREDENTIALS_ST() {}

  REQUEST_CREDENTIALS_ST::~REQUEST_CREDENTIALS_ST() {}

  /////////////////////////////////////////////////////////////////////////////
  RECONCILE_PAYLOAD_ST::RECONCILE_PAYLOAD_ST() {}

  RECONCILE_PAYLOAD_ST::~RECONCILE_PAYLOAD_ST() {}

  /////////////////////////////////////////////////////////////////////////////
  WALLET_INFO_ST::WALLET_INFO_ST() {}

  WALLET_INFO_ST::WALLET_INFO_ST(const WALLET_INFO_ST& other) {
    paymentId_ = other.paymentId_;
    addressBAT_ = other.addressBAT_;
    addressBTC_ = other.addressBTC_;
    addressCARD_ID_ = other.addressCARD_ID_;
    addressETH_ = other.addressETH_;
    addressLTC_ = other.addressLTC_;
    keyInfoSeed_ = other.keyInfoSeed_;
  }

  WALLET_INFO_ST::~WALLET_INFO_ST() {}


  bool WALLET_INFO_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !( d.HasMember("paymentId") && d["paymentId"].IsString() &&
        d.HasMember("addressBAT") && d["addressBAT"].IsString() &&
        d.HasMember("addressBTC") && d["addressBTC"].IsString() &&
        d.HasMember("addressCARD_ID") && d["addressCARD_ID"].IsString() &&
        d.HasMember("addressETH") && d["addressETH"].IsString() &&
        d.HasMember("addressLTC") && d["addressLTC"].IsString() &&
        d.HasMember("keyInfoSeed") && d["keyInfoSeed"].IsString() );
    }

    if (false == error) {
      //convert keyInfoSeed and check error
      std::string sKeyInfoSeed = d["keyInfoSeed"].GetString();
      error = !getFromBase64(sKeyInfoSeed, keyInfoSeed_);
    }

    if (false == error) {
      paymentId_ = d["paymentId"].GetString();
      addressBAT_ = d["addressBAT"].GetString();
      addressBTC_ = d["addressBTC"].GetString();
      addressCARD_ID_ = d["addressCARD_ID"].GetString();
      addressETH_ = d["addressETH"].GetString();
      addressLTC_ = d["addressLTC"].GetString();
    }
    return !error;
  }

  void saveToJson(JsonWriter & writer, const WALLET_INFO_ST& data) {
    writer.StartObject();

    writer.String("paymentId");
    writer.String(data.paymentId_.c_str());

    writer.String("addressBAT");
    writer.String(data.addressBAT_.c_str());

    writer.String("addressBTC");
    writer.String(data.addressBTC_.c_str());

    writer.String("addressCARD_ID");
    writer.String(data.addressCARD_ID_.c_str());

    writer.String("addressETH");
    writer.String(data.addressETH_.c_str());

    writer.String("addressLTC");
    writer.String(data.addressLTC_.c_str());

    writer.String("keyInfoSeed");
    writer.String(getBase64(data.keyInfoSeed_).c_str());

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  UNSIGNED_TX::UNSIGNED_TX() {}

  UNSIGNED_TX::~UNSIGNED_TX() {}

  /////////////////////////////////////////////////////////////////////////////
  TRANSACTION_BALLOT_ST::TRANSACTION_BALLOT_ST() :
    offset_(0) {}

  TRANSACTION_BALLOT_ST::~TRANSACTION_BALLOT_ST() {}

  bool TRANSACTION_BALLOT_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("publisher") && d["publisher"].IsString() &&
        d.HasMember("offset") && d["offset"].IsUint() );
    }

    if (false == error) {
      publisher_ = d["publisher"].GetString();
      offset_ = d["offset"].GetUint();
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const TRANSACTION_BALLOT_ST& data) {
    writer.StartObject();

    writer.String("publisher");
    writer.String(data.publisher_.c_str());

    writer.String("offset");
    writer.Uint(data.offset_);

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  TRANSACTION_ST::TRANSACTION_ST():
    votes_(0) {}

  TRANSACTION_ST::TRANSACTION_ST(const TRANSACTION_ST& transaction) {
    viewingId_ = transaction.viewingId_;
    surveyorId_ = transaction.surveyorId_;
    contribution_fiat_amount_ = transaction.contribution_fiat_amount_;
    contribution_fiat_currency_ = transaction.contribution_fiat_currency_;
    contribution_rates_ = transaction.contribution_rates_;
    contribution_altcurrency_ = transaction.contribution_altcurrency_;
    contribution_probi_ = transaction.contribution_probi_;
    contribution_fee_ = transaction.contribution_fee_;
    submissionStamp_ = transaction.submissionStamp_;
    submissionId_ = transaction.submissionId_;
    contribution_rates_ = transaction.contribution_rates_;
    anonizeViewingId_ = transaction.anonizeViewingId_;
    registrarVK_ = transaction.registrarVK_;
    masterUserToken_ = transaction.masterUserToken_;
    surveyorIds_ = transaction.surveyorIds_;
    satoshis_ = transaction.satoshis_;
    altCurrency_ = transaction.altCurrency_;
    probi_ = transaction.probi_;
    votes_ = transaction.votes_;
    ballots_ = transaction.ballots_;
  }

  TRANSACTION_ST::~TRANSACTION_ST() {}

  bool TRANSACTION_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("viewingId") && d["viewingId"].IsString() &&
        d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
        d.HasMember("contribution_fiat_amount") && d["contribution_fiat_amount"].IsString() &&
        d.HasMember("contribution_fiat_currency") && d["contribution_fiat_currency"].IsString() &&
        d.HasMember("rates") && d["rates"].IsObject() &&
        d["rates"].HasMember("ETH") &&
        d["rates"].HasMember("LTC") &&
        d["rates"].HasMember("BTC") &&
        d["rates"].HasMember("USD") &&
        d["rates"].HasMember("EUR") &&
        d.HasMember("contribution_altcurrency") && d["contribution_altcurrency"].IsString() &&
        d.HasMember("contribution_probi") && d["contribution_probi"].IsString() &&
        d.HasMember("contribution_fee") && d["contribution_fee"].IsString() &&
        d.HasMember("submissionStamp") && d["submissionStamp"].IsString() &&
        d.HasMember("submissionId") && d["submissionId"].IsString() &&
        d.HasMember("anonizeViewingId") && d["anonizeViewingId"].IsString() &&
        d.HasMember("registrarVK") && d["registrarVK"].IsString() &&
        d.HasMember("masterUserToken") && d["masterUserToken"].IsString() &&
        d.HasMember("surveyorIds") && d["surveyorIds"].IsArray() &&
        d.HasMember("satoshis") && d["satoshis"].IsString() &&
        d.HasMember("altCurrency") && d["altCurrency"].IsString() &&
        d.HasMember("probi") && d["probi"].IsString() &&
        d.HasMember("votes") && d["votes"].IsUint() &&
        d.HasMember("ballots") && d["ballots"].IsArray());
    }

    if (false == error) {
      viewingId_ = d["viewingId"].GetString();
      surveyorId_ = d["surveyorId"].GetString();
      contribution_fiat_amount_ = d["contribution_fiat_amount"].GetString();
      contribution_fiat_currency_ = d["contribution_fiat_currency"].GetString();
      contribution_altcurrency_ = d["contribution_altcurrency"].GetString();
      contribution_probi_ = d["contribution_probi"].GetString();
      contribution_fee_ = d["contribution_fee"].GetString();
      submissionStamp_ = d["submissionStamp"].GetString();
      submissionId_ = d["submissionId"].GetString();
      anonizeViewingId_ = d["anonizeViewingId"].GetString();
      registrarVK_ = d["registrarVK"].GetString();
      masterUserToken_ = d["masterUserToken"].GetString();
      satoshis_ = d["satoshis"].GetString();
      altCurrency_ = d["altCurrency"].GetString();
      probi_ = d["probi"].GetString();
      votes_ = d["votes"].GetUint();

      for ( auto & i : d["rates"].GetObject()) {
        contribution_rates_.insert(std::make_pair(i.name.GetString(), i.value.GetDouble() ));
      }

      for (auto & i : d["surveyorIds"].GetArray()) {
        surveyorIds_.push_back(i.GetString());
      }

      for (const auto & i : d["ballots"].GetArray() ) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        TRANSACTION_BALLOT_ST ballot;
        ballot.loadFromJson(sb.GetString());
        ballots_.push_back(ballot);
      }
    }

    return !error;
  }


  void saveToJson(JsonWriter & writer, const TRANSACTION_ST& data) {
    writer.StartObject();

    writer.String("viewingId");
    writer.String(data.viewingId_.c_str());

    writer.String("surveyorId");
    writer.String(data.surveyorId_.c_str());

    writer.String("contribution_fiat_amount");
    writer.String(data.contribution_fiat_amount_.c_str());

    writer.String("contribution_fiat_currency");
    writer.String(data.contribution_fiat_currency_.c_str());

    writer.String("rates");
    writer.StartObject();
    for (auto & p : data.contribution_rates_) {
      writer.String(p.first.c_str());
      writer.Double(p.second);
    }
    writer.EndObject();

    writer.String("contribution_altcurrency");
    writer.String(data.contribution_altcurrency_.c_str());

    writer.String("contribution_probi");
    writer.String(data.contribution_probi_.c_str());

    writer.String("contribution_fee");
    writer.String(data.contribution_fee_.c_str());

    writer.String("submissionStamp");
    writer.String(data.submissionStamp_.c_str());

    writer.String("submissionId");
    writer.String(data.submissionId_.c_str());

    writer.String("anonizeViewingId");
    writer.String(data.anonizeViewingId_.c_str());

    writer.String("registrarVK");
    writer.String(data.registrarVK_.c_str());

    writer.String("masterUserToken");
    writer.String(data.masterUserToken_.c_str());

    writer.String("surveyorIds");
    writer.StartArray();
    for (auto & i : data.surveyorIds_) {
      writer.String(i.c_str());
    }
    writer.EndArray();

    writer.String("satoshis");
    writer.String(data.satoshis_.c_str());

    writer.String("altCurrency");
    writer.String(data.altCurrency_.c_str());

    writer.String("probi");
    writer.String(data.probi_.c_str());

    writer.String("votes");
    writer.Uint(data.votes_);

    /* TODO: clarify if it needs to be serialized
    writer.String("ballots");
    writer.StartArray();
    for (auto & i : data.ballots_) {
      saveToJson(writer, i);
    }
    writer.EndArray();
    */

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  BALLOT_ST::BALLOT_ST():
    offset_(0),
    delayStamp_(0) {}

  BALLOT_ST::BALLOT_ST(const BALLOT_ST& ballot) {
    viewingId_ = ballot.viewingId_;
    surveyorId_ = ballot.surveyorId_;
    publisher_ = ballot.publisher_;
    offset_ = ballot.offset_;
    prepareBallot_ = ballot.prepareBallot_;
    proofBallot_ = ballot.proofBallot_;
    delayStamp_ = ballot.delayStamp_;
  }

  BALLOT_ST::~BALLOT_ST() {}

  bool BALLOT_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("viewingId") &&  d["viewingId"].IsString() &&
        d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
        d.HasMember("publisher") && d["publisher"].IsString() &&
        d.HasMember("offset") && d["offset"].IsUint() &&
        d.HasMember("prepareBallot") && d["prepareBallot"].IsString() &&
        d.HasMember("delayStamp") && d["delayStamp"].IsUint64() );
    }

    if (false == error) {
      viewingId_ = d["viewingId"].GetString();
      surveyorId_ = d["surveyorId"].GetString();
      publisher_ = d["publisher"].GetString();
      offset_ = d["offset"].GetUint();
      prepareBallot_ = d["prepareBallot"].GetString();
      delayStamp_ = d["delayStamp"].GetUint64();
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const BALLOT_ST& data) {
    writer.StartObject();

    writer.String("viewingId");
    writer.String(data.viewingId_.c_str());

    writer.String("surveyorId");
    writer.String(data.surveyorId_.c_str());

    writer.String("publisher");
    writer.String(data.publisher_.c_str());

    writer.String("offset");
    writer.Uint(data.offset_);

    writer.String("prepareBallot");
    writer.String(data.prepareBallot_.c_str());

    writer.String("delayStamp");
    writer.Uint64(data.delayStamp_);

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  BATCH_VOTES_INFO_ST::BATCH_VOTES_INFO_ST() {}

  BATCH_VOTES_INFO_ST::BATCH_VOTES_INFO_ST(const BATCH_VOTES_INFO_ST& other) {
    surveyorId_ = other.surveyorId_;
    proof_ = other.proof_;
  }

  BATCH_VOTES_INFO_ST::~BATCH_VOTES_INFO_ST() {}

  bool BATCH_VOTES_INFO_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    // Has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
        d.HasMember("proof") && d["proof"].IsString());
    }

    if (false == error) {
      surveyorId_ = d["surveyorId"].GetString();
      proof_ = d["proof"].GetString();
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const BATCH_VOTES_INFO_ST& data) {
    writer.StartObject();

    writer.String("surveyorId");
    writer.String(data.surveyorId_.c_str());

    writer.String("proof");
    writer.String(data.proof_.c_str());

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  BATCH_VOTES_ST::BATCH_VOTES_ST() {}

  BATCH_VOTES_ST::BATCH_VOTES_ST(const BATCH_VOTES_ST& other) {
    publisher_ = other.publisher_;
    batchVotesInfo_ = other.batchVotesInfo_;
  }

  BATCH_VOTES_ST::~BATCH_VOTES_ST() {}

  bool BATCH_VOTES_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    // Has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("publisher") &&  d["publisher"].IsString() &&
        d.HasMember("batchVotesInfo") && d["batchVotesInfo"].IsArray());
    }

    if (false == error) {
      publisher_ = d["publisher"].GetString();
      for (const auto & i : d["batchVotesInfo"].GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        BATCH_VOTES_INFO_ST b;
        b.loadFromJson(sb.GetString());
        batchVotesInfo_.push_back(b);
      }
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const BATCH_VOTES_ST& data) {
    writer.StartObject();

    writer.String("publisher");
    writer.String(data.publisher_.c_str());

    writer.String("batchVotesInfo");
    writer.StartArray();
    for (auto & b : data.batchVotesInfo_) {
      saveToJson(writer, b);
    }
    writer.EndArray();

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  CLIENT_STATE_ST::CLIENT_STATE_ST():
    bootStamp_(0),
    reconcileStamp_(0),
    settings_(AD_FREE_SETTINGS),
    fee_amount_(0),
    user_changed_fee_(false),
    days_(0),
    auto_contribute_(false),
    rewards_enabled_(false) {}

  CLIENT_STATE_ST::CLIENT_STATE_ST(const CLIENT_STATE_ST& other) {
    walletInfo_ = other.walletInfo_;
    bootStamp_ = other.bootStamp_;
    reconcileStamp_ = other.reconcileStamp_;
    personaId_ = other.personaId_;
    userId_ = other.userId_;
    registrarVK_ = other.registrarVK_;
    masterUserToken_ = other.masterUserToken_;
    preFlight_ = other.preFlight_;
    fee_currency_ = other.fee_currency_;
    settings_ = other.settings_;
    fee_amount_ = other.fee_amount_;
    user_changed_fee_ = other.user_changed_fee_;
    days_ = other.days_;
    transactions_ = other.transactions_;
    ballots_ = other.ballots_;
    ruleset_ = other.ruleset_;
    rulesetV2_ = other.rulesetV2_;
    batch_ = other.batch_;
    auto_contribute_ = other.auto_contribute_;
    rewards_enabled_ = other.rewards_enabled_;
  }

  CLIENT_STATE_ST::~CLIENT_STATE_ST() {}

  bool CLIENT_STATE_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser error or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("walletInfo") && d["walletInfo"].IsObject() &&
        d.HasMember("bootStamp") && d["bootStamp"].IsUint64() &&
        d.HasMember("reconcileStamp") && d["reconcileStamp"].IsUint64() &&
        d.HasMember("personaId") && d["personaId"].IsString() &&
        d.HasMember("userId") && d["userId"].IsString() &&
        d.HasMember("registrarVK") && d["registrarVK"].IsString() &&
        d.HasMember("masterUserToken") && d["masterUserToken"].IsString() &&
        d.HasMember("preFlight") && d["preFlight"].IsString() &&
        d.HasMember("fee_currency") && d["fee_currency"].IsString() &&
        d.HasMember("settings") && d["settings"].IsString() &&
        d.HasMember("fee_amount") && d["fee_amount"].IsDouble() &&
        d.HasMember("user_changed_fee") && d["user_changed_fee"].IsBool() &&
        d.HasMember("days") && d["days"].IsUint() &&
        d.HasMember("transactions") && d["transactions"].IsArray() &&
        d.HasMember("ballots") && d["ballots"].IsArray() &&
        d.HasMember("ruleset") && d["ruleset"].IsString() &&
        d.HasMember("rulesetV2") && d["rulesetV2"].IsString() &&
        d.HasMember("batch") && d["batch"].IsArray() &&
        d.HasMember("auto_contribute") && d["auto_contribute"].IsBool() &&
        d.HasMember("rewards_enabled") && d["rewards_enabled"].IsBool()
      );
    }

    if (false == error) {
      {
        auto & i = d["walletInfo"];
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);
        walletInfo_.loadFromJson(sb.GetString());
      }

      bootStamp_ = d["bootStamp"].GetUint64();
      reconcileStamp_ = d["reconcileStamp"].GetUint64();
      personaId_ = d["personaId"].GetString();
      userId_ = d["userId"].GetString();
      registrarVK_ = d["registrarVK"].GetString();
      masterUserToken_ = d["masterUserToken"].GetString();
      preFlight_ = d["preFlight"].GetString();
      fee_currency_ = d["fee_currency"].GetString();
      settings_ = d["settings"].GetString();
      fee_amount_ = d["fee_amount"].GetDouble();
      user_changed_fee_ = d["user_changed_fee"].GetBool();
      days_ = d["days"].GetUint();
      auto_contribute_ = d["auto_contribute"].GetBool();
      rewards_enabled_ = d["rewards_enabled"].GetBool();

      for (const auto & i : d["transactions"].GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        TRANSACTION_ST ta;
        ta.loadFromJson(sb.GetString());
        transactions_.push_back(ta);
      }

      for (const auto & i : d["ballots"].GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        BALLOT_ST b;
        b.loadFromJson(sb.GetString());
        ballots_.push_back(b);
      }

      ruleset_ = d["ruleset"].GetString();
      rulesetV2_ = d["rulesetV2"].GetString();

      for (const auto & i : d["batch"].GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        BATCH_VOTES_ST b;
        b.loadFromJson(sb.GetString());
        batch_.push_back(b);
      }
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const CLIENT_STATE_ST& data) {
    writer.StartObject();

    writer.String("walletInfo");
    saveToJson(writer, data.walletInfo_);

    writer.String("bootStamp");
    writer.Uint64(data.bootStamp_);

    writer.String("reconcileStamp");
    writer.Uint64(data.reconcileStamp_);

    writer.String("personaId");
    writer.String(data.personaId_.c_str());

    writer.String("userId");
    writer.String(data.userId_.c_str());

    writer.String("registrarVK");
    writer.String(data.registrarVK_.c_str());

    writer.String("masterUserToken");
    writer.String(data.masterUserToken_.c_str());

    writer.String("preFlight");
    writer.String(data.preFlight_.c_str());

    writer.String("fee_currency");
    writer.String(data.fee_currency_.c_str());

    writer.String("settings");
    writer.String(data.settings_.c_str());

    writer.String("fee_amount");
    writer.Double(data.fee_amount_);

    writer.String("user_changed_fee");
    writer.Bool(data.user_changed_fee_);

    writer.String("days");
    writer.Uint(data.days_);

    writer.String("rewards_enabled");
    writer.Bool(data.rewards_enabled_);

    writer.String("auto_contribute");
    writer.Bool(data.auto_contribute_);

    writer.String("transactions");
    writer.StartArray();
    for (auto & t : data.transactions_) {
      saveToJson(writer, t);
    }
    writer.EndArray();

    writer.String("ballots");
    writer.StartArray();
    for (auto & b : data.ballots_) {
      saveToJson(writer, b);
    }
    writer.EndArray();

    writer.String("ruleset");
    writer.String(data.ruleset_.c_str());

    writer.String("rulesetV2");
    writer.String(data.rulesetV2_.c_str());

    writer.String("batch");
    writer.StartArray();
    for (auto & b : data.batch_) {
      saveToJson(writer, b);
    }
    writer.EndArray();

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  REPORT_BALANCE_ST::REPORT_BALANCE_ST():
    opening_balance_(.0),
    closing_balance_(.0),
    grants_(.0),
    earning_from_ads_(.0),
    auto_contribute_(.0),
    recurring_donation_(.0),
    one_time_donation_(.0) {}

  REPORT_BALANCE_ST::~REPORT_BALANCE_ST() {}

  bool REPORT_BALANCE_ST::loadFromJson(const std::string& json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("opening_balance") && d["opening_balance"].IsDouble() &&
        d.HasMember("closing_balance") && d["closing_balance"].IsDouble() &&
        d.HasMember("grants") && d["grants"].IsDouble() &&
        d.HasMember("earning_from_ads") && d["earning_from_ads"].IsDouble() &&
        d.HasMember("auto_contribute") && d["auto_contribute"].IsDouble() &&
        d.HasMember("recurring_donation") && d["recurring_donation"].IsDouble() &&
        d.HasMember("one_time_donation") && d["one_time_donation"].IsDouble());
    }

    if (false == error) {
      opening_balance_ = d["opening_balance"].GetDouble();
      closing_balance_ = d["closing_balance"].GetDouble();
      grants_ = d["grants"].GetDouble();
      earning_from_ads_ = d["earning_from_ads"].GetDouble();
      auto_contribute_ = d["auto_contribute"].GetDouble();
      recurring_donation_ = d["recurring_donation"].GetDouble();
      one_time_donation_ = d["one_time_donation"].GetDouble();
    }

    return !error;
  }

  void saveToJson(JsonWriter& writer, const REPORT_BALANCE_ST& data) {
    writer.StartObject();

    writer.String("opening_balance");
    writer.Double(data.opening_balance_);

    writer.String("closing_balance");
    writer.Double(data.closing_balance_);

    writer.String("grants");
    writer.Double(data.grants_);

    writer.String("earning_from_ads");
    writer.Double(data.earning_from_ads_);

    writer.String("auto_contribute");
    writer.Double(data.auto_contribute_);

    writer.String("recurring_donation");
    writer.Double(data.recurring_donation_);

    writer.String("one_time_donation");
    writer.Double(data.one_time_donation_);

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  PUBLISHER_STATE_ST::PUBLISHER_STATE_ST():
    min_pubslisher_duration_(braveledger_ledger::_default_min_pubslisher_duration),
    min_visits_(1),
    allow_non_verified_(true),
    pubs_load_timestamp_ (0ull),
    allow_videos_(true) {}

  PUBLISHER_STATE_ST::PUBLISHER_STATE_ST(const PUBLISHER_STATE_ST& state) {
    min_pubslisher_duration_ = state.min_pubslisher_duration_;
    min_visits_ = state.min_visits_;
    allow_non_verified_ = state.allow_non_verified_;
    pubs_load_timestamp_ = state.pubs_load_timestamp_;
    allow_videos_ = state.allow_videos_;
    monthly_balances_ = state.monthly_balances_;
    recurring_donation_ = state.recurring_donation_;
  }

  PUBLISHER_STATE_ST::~PUBLISHER_STATE_ST() {}

  bool PUBLISHER_STATE_ST::loadFromJson(const std::string& json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("min_pubslisher_duration") && d["min_pubslisher_duration"].IsUint() &&
        d.HasMember("min_visits") && d["min_visits"].IsUint() &&
        d.HasMember("allow_non_verified") && d["allow_non_verified"].IsBool() &&
        d.HasMember("pubs_load_timestamp") && d["pubs_load_timestamp"].IsUint64() &&
        d.HasMember("allow_videos") && d["allow_videos"].IsBool() &&
        d.HasMember("monthly_balances") && d["monthly_balances"].IsArray() &&
        d.HasMember("recurring_donation") && d["recurring_donation"].IsArray());
    }

    if (false == error) {
      min_pubslisher_duration_ = d["min_pubslisher_duration"].GetUint();
      min_visits_ = d["min_visits"].GetUint();
      allow_non_verified_ = d["allow_non_verified"].GetBool();
      pubs_load_timestamp_ = d["pubs_load_timestamp"].GetUint64();
      allow_videos_ = d["allow_videos"].GetBool();

      for (const auto & i : d["monthly_balances"].GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        rapidjson::Document d1;
        d1.Parse(sb.GetString());

        rapidjson::Value::ConstMemberIterator itr = d1.MemberBegin();
        if (itr != d1.MemberEnd()) {
          rapidjson::StringBuffer sb1;
          rapidjson::Writer<rapidjson::StringBuffer> writer1(sb1);
          itr->value.Accept(writer1);
          REPORT_BALANCE_ST r;
          r.loadFromJson(sb1.GetString());
          monthly_balances_.insert(std::make_pair(itr->name.GetString(), r));
        }
      }
      for (const auto & i : d["recurring_donation"].GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        rapidjson::Document d1;
        d1.Parse(sb.GetString());

        rapidjson::Value::ConstMemberIterator itr = d1.MemberBegin();
        if (itr != d1.MemberEnd()) {
          recurring_donation_.insert(std::make_pair(itr->name.GetString(), itr->value.GetDouble()));
        }
      }
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const PUBLISHER_STATE_ST& data) {
    writer.StartObject();

    writer.String("min_pubslisher_duration");
    writer.Uint(data.min_pubslisher_duration_);

    writer.String("min_visits");
    writer.Uint(data.min_visits_);

    writer.String("allow_non_verified");
    writer.Bool(data.allow_non_verified_);

    writer.String("pubs_load_timestamp");
    writer.Uint64(data.pubs_load_timestamp_);

    writer.String("allow_videos");
    writer.Bool(data.allow_videos_);

    writer.String("monthly_balances");
    writer.StartArray();
    for (auto & p : data.monthly_balances_) {
      writer.StartObject();
      writer.String(p.first.c_str());
      saveToJson(writer, p.second);
      writer.EndObject();
    }
    writer.EndArray();

    writer.String("recurring_donation");
    writer.StartArray();
    for (auto & p : data.recurring_donation_) {
      writer.StartObject();
      writer.String(p.first.c_str());
      writer.Double(p.second);
      writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  PUBLISHER_ST::PUBLISHER_ST():
    id_(""),
    duration_(0),
    score_(0),
    visits_(0),
    percent_(0),
    weight_(0) {}

  PUBLISHER_ST::~PUBLISHER_ST() {}

  bool PUBLISHER_ST::operator<(const PUBLISHER_ST& rhs) const {
    return score_ > rhs.score_;
  }
  /////////////////////////////////////////////////////////////////////////////
  WINNERS_ST::WINNERS_ST() :
    votes_(0) {}

  WINNERS_ST::~WINNERS_ST() {}

  /////////////////////////////////////////////////////////////////////////////
  WALLET_PROPERTIES_ST::WALLET_PROPERTIES_ST() : balance_(0), parameters_days_(0) {}

  WALLET_PROPERTIES_ST::~WALLET_PROPERTIES_ST() {}

  WALLET_PROPERTIES_ST::WALLET_PROPERTIES_ST(const WALLET_PROPERTIES_ST &properties) {
    altcurrency_ = properties.altcurrency_;
    probi_ = properties.probi_;
    balance_ = properties.balance_;
    rates_ = properties.rates_;
    fee_amount_ = properties.fee_amount_;
    parameters_choices_ = properties.parameters_choices_;
    parameters_range_ = properties.parameters_range_;
    parameters_days_ = properties.parameters_days_;
    grants_ = properties.grants_;
  }

  bool WALLET_PROPERTIES_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(
        d.HasMember("altcurrency") && d["altcurrency"].IsString() &&
        d.HasMember("balance") && d["balance"].IsString() &&
        d.HasMember("probi") && d["probi"].IsString() &&
        d.HasMember("rates") && d["rates"].IsObject() &&
        d.HasMember("parameters") && d["parameters"].IsObject()
      );
    }

    if (false == error) {
      altcurrency_ = d["altcurrency"].GetString();
      balance_ = std::stod(d["balance"].GetString());
      probi_ = d["probi"].GetString();

      for (auto & i : d["rates"].GetObject()) {
        double value = 0.0;

        // For some reason BTC is returned as string, where others are double
        if (i.value.IsDouble()) {
          value = i.value.GetDouble();
        } else if (i.value.IsString()) {
          value = std::stod(i.value.GetString());
        }
        rates_.insert(std::make_pair(i.name.GetString(), value));
      }

      for (auto & i : d["parameters"]["adFree"]["choices"]["BAT"].GetArray()) {
        parameters_choices_.push_back(i.GetDouble());
      }

      for (auto & i : d["parameters"]["adFree"]["range"]["BAT"].GetArray()) {
        parameters_range_.push_back(i.GetDouble());
      }

      parameters_days_ = d["parameters"]["adFree"]["days"].GetUint();
      fee_amount_ = d["parameters"]["adFree"]["fee"]["BAT"].GetDouble();

      if (d.HasMember("grants") && d["grants"].IsArray()) {
        for (auto &i : d["grants"].GetArray()) {
          GRANT grant;
          auto obj = i.GetObject();
          if (obj.HasMember("probi")) {
            grant.probi = obj["probi"].GetString();
          }

          if (obj.HasMember("altcurrency")) {
            grant.altcurrency = obj["altcurrency"].GetString();
          }

          if (obj.HasMember("expiryTime")) {
            grant.expiryTime = obj["expiryTime"].GetUint64();
          }

          grants_.push_back(grant);
        }
      } else {
        grants_.clear();
      }
    }
    return !error;
  }

  /////////////////////////////////////////////////////////////////////////////
  GRANT::GRANT() : expiryTime(0) {}

  GRANT::~GRANT() {}

  GRANT::GRANT(const GRANT &properties) {
    promotionId = properties.promotionId;
    altcurrency = properties.altcurrency;
    expiryTime = properties.expiryTime;
    probi = properties.probi;
  }

  bool GRANT::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (error == true) {
      return !error;
    }

    // First grant get
    error = !(
        d.HasMember("promotionId") && d["promotionId"].IsString()
    );

    if (error == false) {
      promotionId = d["promotionId"].GetString();
      return !error;
    }

    // On successful grant
    error = !(
        d.HasMember("altcurrency") && d["altcurrency"].IsString() &&
        d.HasMember("expiryTime") && d["expiryTime"].IsNumber() &&
        d.HasMember("probi") && d["probi"].IsString()
    );

    if (error == false) {
      altcurrency = d["altcurrency"].GetString();
      expiryTime = d["expiryTime"].GetUint64();
      probi = d["probi"].GetString();
    }

    return !error;
  }

  /////////////////////////////////////////////////////////////////////////////
  SURVEYOR_INFO_ST::SURVEYOR_INFO_ST() {}

  SURVEYOR_INFO_ST::~SURVEYOR_INFO_ST() {}

 /////////////////////////////////////////////////////////////////////////////
  CURRENT_RECONCILE::CURRENT_RECONCILE() :
    timestamp_(0) {}

  CURRENT_RECONCILE::~CURRENT_RECONCILE() {}

  /////////////////////////////////////////////////////////////////////////////
  SURVEYOR_ST::SURVEYOR_ST() {}

  SURVEYOR_ST::~SURVEYOR_ST() {}

  bool SURVEYOR_ST::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("signature") && d["signature"].IsString() &&
        d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
        d.HasMember("surveyVK") && d["surveyVK"].IsString() &&
        d.HasMember("registrarVK") && d["registrarVK"].IsString());
    }

    if (false == error) {
      signature_ = d["signature"].GetString();
      surveyorId_ = d["surveyorId"].GetString();
      surveyVK_ = d["surveyVK"].GetString();
      registrarVK_ = d["registrarVK"].GetString();
      if (d.HasMember("surveySK") && d["surveySK"].IsString()) {
        surveySK_ = d["surveySK"].GetString();
      }
    }

    return !error;
  }

  void saveToJson(JsonWriter & writer, const SURVEYOR_ST& data) {
    writer.StartObject();

    writer.String("signature");
    writer.String(data.signature_.c_str());

    writer.String("surveyorId");
    writer.String(data.surveyorId_.c_str());

    writer.String("surveyVK");
    writer.String(data.surveyVK_.c_str());

    writer.String("registrarVK");
    writer.String(data.registrarVK_.c_str());

    writer.String("surveySK");
    writer.String(data.surveySK_.c_str());

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  TWITCH_EVENT_INFO::TWITCH_EVENT_INFO() {}

  TWITCH_EVENT_INFO::TWITCH_EVENT_INFO(const TWITCH_EVENT_INFO& twitchEventInfo):
    event_(twitchEventInfo.event_),
    time_(twitchEventInfo.time_),
    status_(twitchEventInfo.status_) {}

  TWITCH_EVENT_INFO::~TWITCH_EVENT_INFO() {}

/////////////////////////////////////////////////////////////////////////////
  MEDIA_PUBLISHER_INFO::MEDIA_PUBLISHER_INFO() {}

  MEDIA_PUBLISHER_INFO::MEDIA_PUBLISHER_INFO(const MEDIA_PUBLISHER_INFO& mediaPublisherInfo):
    publisherName_(mediaPublisherInfo.publisherName_),
    publisherURL_(mediaPublisherInfo.publisherURL_),
    favIconURL_(mediaPublisherInfo.favIconURL_),
    channelName_(mediaPublisherInfo.channelName_),
    publisher_id_(mediaPublisherInfo.publisher_id_),
    twitchEventInfo_(mediaPublisherInfo.twitchEventInfo_) {}

  MEDIA_PUBLISHER_INFO::~MEDIA_PUBLISHER_INFO() {}


  bool MEDIA_PUBLISHER_INFO::loadFromJson(const std::string & json) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("publisherName") && d["publisherName"].IsString() &&
        d.HasMember("publisherURL") && d["publisherURL"].IsString() &&
        d.HasMember("favIconURL") && d["favIconURL"].IsString() &&
        d.HasMember("channelName") && d["channelName"].IsString() &&
        d.HasMember("publisherId") && d["publisherId"].IsString() &&
        d.HasMember("twitch_event") && d["twitch_event"].IsString() &&
        d.HasMember("twitch_time") && d["twitch_time"].IsString() &&
        d.HasMember("twitch_status") && d["twitch_status"].IsString());
    }

    if (false == error) {
      publisherName_ = d["publisherName"].GetString();
      publisherURL_ = d["publisherURL"].GetString();
      favIconURL_ = d["favIconURL"].GetString();
      channelName_ = d["channelName"].GetString();
      publisher_id_ = d["publisherId"].GetString();
      twitchEventInfo_.event_ = d["twitch_event"].GetString();
      twitchEventInfo_.time_ = d["twitch_time"].GetString();
      twitchEventInfo_.status_ = d["twitch_status"].GetString();
    }
    return !error;
  }

  void saveToJson(JsonWriter & writer, const MEDIA_PUBLISHER_INFO& data) {
    writer.StartObject();

    writer.String("publisherName");
    writer.String(data.publisherName_.c_str());

    writer.String("publisherURL");
    writer.String(data.publisherURL_.c_str());

    writer.String("favIconURL");
    writer.String(data.favIconURL_.c_str());

    writer.String("channelName");
    writer.String(data.channelName_.c_str());

    writer.String("publisherId");
    writer.String(data.publisher_id_.c_str());

    writer.String("twitch_event");
    writer.String(data.twitchEventInfo_.event_.c_str());

    writer.String("twitch_time");
    writer.String(data.twitchEventInfo_.time_.c_str());

    writer.String("twitch_status");
    writer.String(data.twitchEventInfo_.status_.c_str());

    writer.EndObject();
  }

/////////////////////////////////////////////////////////////////////////////
  BATCH_PROOF::BATCH_PROOF() {}

  BATCH_PROOF::~BATCH_PROOF() {}

/////////////////////////////////////////////////////////////////////////////
  void split(std::vector<std::string>& tmp, std::string query, char delimiter) {
    std::stringstream ss(query);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
      if (query[0] != '\n') {
        tmp.push_back(item);
      }
    }
  }

  bool getJSONValue(const std::string& fieldName, const std::string& json, std::string & value) {

    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError() || ( false == d.HasMember(fieldName.c_str()) );
    if (false == error) {
      value = d[fieldName.c_str()].GetString();
    }
    return !error;
}

  bool getJSONList(const std::string& fieldName, const std::string& json, std::vector<std::string> & value) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError() || (false == ( d.HasMember(fieldName.c_str())  &&  d[fieldName.c_str()].IsArray() ) );
    if (false == error) {
      for (auto & i : d[fieldName.c_str()].GetArray()) {
        value.push_back(i.GetString());
      }
    }
    return !error;
  }

  bool getJSONTwitchProperties(const std::string& json, std::vector<std::map<std::string, std::string>>& parts) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      for (auto & i : d.GetArray()) {
        const char * event_field = "event";
        std::map<std::string, std::string> eventmap;

        auto obj = i.GetObject();
        if (obj.HasMember(event_field)) {
          eventmap[event_field] = obj[event_field].GetString();
        }

        const char * props_field = "properties";
        if (obj.HasMember(props_field)) {
          eventmap[props_field] = "";

          const char * channel_field = "channel";
          if (obj[props_field].HasMember(channel_field) &&
            obj[props_field][channel_field].IsString()) {
            eventmap[channel_field] = obj[props_field][channel_field].GetString();
          }

          const char * vod_field = "vod";
          if (obj[props_field].HasMember(vod_field)) {
            eventmap[vod_field] = obj[props_field][vod_field].GetString();
          }

          const char * time_field = "time";
          if (obj[props_field].HasMember(time_field)) {
            double d = obj[props_field][time_field].GetDouble();
            eventmap[time_field] = std::to_string(d);
          }
        }
        parts.push_back(eventmap);
      } //for (auto & i : d.GetArray())

    }
    return !error;
  }

  bool getJSONBatchSurveyors(const std::string& json, std::vector<std::string>& surveyors) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      for (auto & i : d.GetArray()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);
        std::string surveyor = sb.GetString();
        surveyors.push_back(surveyor);
      }
    }
    return !error;
  }

  bool getJSONRates(const std::string& json, std::map<std::string, double>& rates) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("rates") && d["rates"].IsObject() &&
        d["rates"].HasMember("ETH") &&
        d["rates"].HasMember("LTC") &&
        d["rates"].HasMember("BTC") &&
        d["rates"].HasMember("USD") &&
        d["rates"].HasMember("EUR"));
    }

    if (false == error) {
      for (auto & i : d["rates"].GetObject()) {
        rates.insert(std::make_pair(i.name.GetString(), i.value.GetDouble()));
      }
    }
    return !error;
  }

  bool getJSONTransaction(const std::string& json, TRANSACTION_ST& transaction) {

    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("paymentStamp") && d["paymentStamp"].IsUint64() &&
        d.HasMember("probi") && d["probi"].IsString() &&
        d.HasMember("altcurrency") && d["altcurrency"].IsString() );
    }

    if (false == error) {
      unsigned long long stamp = d["paymentStamp"].GetUint64();
      transaction.submissionStamp_ = std::to_string(stamp);
      transaction.contribution_probi_ = d["probi"].GetString();
      transaction.contribution_altcurrency_ = d["altcurrency"].GetString();
    }
    return !error;
  }

  bool getJSONUnsignedTx(const std::string& json, UNSIGNED_TX& unsignedTx) {
   rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !d.HasMember("unsignedTx") || !(d["unsignedTx"].IsObject());
    }

    if (false == error) {
      unsignedTx.amount_ = d["unsignedTx"]["denomination"]["amount"].GetString();
      unsignedTx.currency_ = d["unsignedTx"]["denomination"]["currency"].GetString();
      unsignedTx.destination_ = d["unsignedTx"]["destination"].GetString();
    }
    return !error;
  }

  bool getJSONPublisherVerified(const std::string& json, bool& verified) {
    verified = false;

    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("properties") && d["properties"].IsObject());
    }

    if (false == error) {
      verified = d["properties"]["verified"].GetBool();
    }
    return !error;
  }

  bool getJSONWalletInfo(const std::string& json, WALLET_INFO_ST& walletInfo,
        std::string& fee_currency, double& fee_amount, unsigned int& days) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("wallet") && d["wallet"].IsObject() &&
        d.HasMember("payload") && d["payload"].IsObject() );
    }

    if (false == error) {
      walletInfo.paymentId_ = d["wallet"]["paymentId"].GetString();
      walletInfo.addressBAT_ = d["wallet"]["addresses"]["BAT"].GetString();
      walletInfo.addressBTC_ = d["wallet"]["addresses"]["BTC"].GetString();
      walletInfo.addressCARD_ID_ = d["wallet"]["addresses"]["CARD_ID"].GetString();
      walletInfo.addressETH_ = d["wallet"]["addresses"]["ETH"].GetString();
      walletInfo.addressLTC_ = d["wallet"]["addresses"]["LTC"].GetString();

      days = d["payload"]["adFree"]["days"].GetUint();
      const auto & fee = d["payload"]["adFree"]["fee"].GetObject();
      auto itr = fee.MemberBegin();
      if (itr != fee.MemberEnd() ) {
        fee_currency = itr->name.GetString();
        fee_amount = itr->value.GetDouble();
      }
    }
    return !error;
  }

  bool getJSONRecoverWallet(const std::string& json, double& balance, std::string& probi, std::vector<GRANT>& grants) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error) {
      error = !(d.HasMember("balance") && d["balance"].IsString() &&
                d.HasMember("probi") && d["probi"].IsString() );
    }

    if (false == error) {
      balance = std::stod(d["balance"].GetString());
      probi = d["probi"].GetString();

      if (d.HasMember("grants") && d["grants"].IsArray()) {
        for (auto &i : d["grants"].GetArray()) {
          GRANT grant;
          auto obj = i.GetObject();
          if (obj.HasMember("probi")) {
            grant.probi = obj["probi"].GetString();
          }

          if (obj.HasMember("altcurrency")) {
            grant.altcurrency = obj["altcurrency"].GetString();
          }

          if (obj.HasMember("expiryTime")) {
            grant.expiryTime = obj["expiryTime"].GetUint64();
          }

          grants.push_back(grant);
        }
      } else {
        grants.clear();
      }
    }
    return !error;
  }

  bool getJSONResponse(const std::string& json, unsigned int& statusCode, std::string& error) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool hasError = d.HasParseError();
    if (hasError == false) {
      hasError = !(d.HasMember("statusCode") && d["statusCode"].IsNumber() &&
                d.HasMember("error") && d["error"].IsString());
    }

    if (hasError == false) {
      statusCode = d["statusCode"].GetUint();
      error = d["error"].GetString();
    }
    return !hasError;
  }

  bool getJSONGrant(const std::string& json, uint64_t& expiryTime) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool hasError = d.HasParseError();
    if (hasError == false) {
      hasError = !(d.HasMember("expiryTime") && d["expiryTime"].IsNumber());
    }

    if (hasError == false) {
      expiryTime = d["expiryTime"].GetUint();
    }
    return !hasError;
  }

  bool getJSONServerList(const std::string& json, std::map<std::string, SERVER_LIST>& list) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    bool hasError = d.HasParseError();
    if (hasError == false) {
      hasError = !d.IsArray();
    }

    list = {};

    if (hasError == false) {
      for (auto &i : d.GetArray()) {
        SERVER_LIST item;
        item.verified = i[1].GetBool();
        item.excluded = i[2].GetBool();
        list.emplace(i[0].GetString(), item);
      }
    }

    return !hasError;
  }

  std::vector<uint8_t> generateSeed() {
    //std::ostringstream seedStr;

    std::vector<uint8_t> vSeed(SEED_LENGTH);
    std::random_device r;
    std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    auto rand = std::bind(std::uniform_int_distribution<>(0, UCHAR_MAX), std::mt19937(seed));

    std::generate_n(vSeed.begin(), SEED_LENGTH, rand);
    return vSeed;
  }

  std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed) {
    DCHECK(!seed.empty());
    std::vector<uint8_t> out(SEED_LENGTH);

    const uint8_t info[] = {0};
    int hkdfRes = HKDF(&out.front(), SEED_LENGTH, EVP_sha512(), &seed.front(), seed.size(),
      braveledger_ledger::g_hkdfSalt, SALT_LENGTH, info, sizeof(info) / sizeof(info[0]));

    DCHECK(hkdfRes);
    DCHECK(!seed.empty());

    // We set the key_length to the length of the expected output and then take
    // the result from the first key, which is the client write key.
    //const std::string key((char*)&seed[0], seed.size());
    //const std::string salt((char*)&braveledger_ledger::g_hkdfSalt[0], SALT_LENGTH);
    //crypto::HKDF hkdf(key, salt, "", SEED_LENGTH, 0, 0);

    //LOG(ERROR) << "hkdf.client_write_key() == " << hkdf.client_write_key().data();
    //memcpy(&out.front(), hkdf.client_write_key().data(), hkdf.client_write_key().size());

    return out;
  }

  void getPublicKeyFromSeed(const std::vector<uint8_t>& seed,
        std::vector<uint8_t>& publicKey, std::vector<uint8_t>& secretKey) {
    DCHECK(!seed.empty());
    publicKey.resize(crypto_sign_PUBLICKEYBYTES);
    secretKey = seed;
    secretKey.resize(crypto_sign_SECRETKEYBYTES);

    crypto_sign_keypair(&publicKey.front(), &secretKey.front(), 1);

    DCHECK(!publicKey.empty() && !secretKey.empty());
  }

  std::string uint8ToHex(const std::vector<uint8_t>& in) {
    std::ostringstream res;
    for (size_t i = 0; i < in.size(); i++) {
      res << std::setfill('0') << std::setw(sizeof(uint8_t) * 2)
         << std::hex << (int)in[i];
    }
    return res.str();
  }


  std::string stringifyBatch(std::vector<BATCH_VOTES_INFO_ST> payload) {
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);

    writer.StartArray();
    for (auto & d : payload) {
      saveToJson(writer, d);
    }
    writer.EndArray();

    return buffer.GetString();
  }

  std::string stringify(std::string* keys, std::string* values, const unsigned int& size) {
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();

    for (unsigned int i = 0; i < size; i++) {
      writer.String(keys[i].c_str());
      writer.String(values[i].c_str());
    }

    writer.EndObject();
    return buffer.GetString();
  }

  std::string stringifyUnsignedTx(const UNSIGNED_TX& unsignedTx) {
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();

    writer.String("denomination");
    writer.StartObject();

    writer.String("amount");
    writer.String(unsignedTx.amount_.c_str());

    writer.String("currency");
    writer.String(unsignedTx.currency_.c_str());
    writer.EndObject();

    writer.String("destination");
    writer.String(unsignedTx.destination_.c_str());

    writer.EndObject();
    return buffer.GetString();
  }

  std::string stringifyRequestCredentialsSt(const REQUEST_CREDENTIALS_ST& request_credentials) {

    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject(); //root

    writer.String("requestType");
    writer.String(request_credentials.requestType_.c_str());

    writer.String("request");
    writer.StartObject(); //request

    writer.String("headers");
    writer.StartObject(); //headers

    writer.String("digest");
    writer.String(request_credentials.request_headers_digest_.c_str());

    writer.String("signature");
    writer.String(request_credentials.request_headers_signature_.c_str());

    writer.EndObject(); //headers

    writer.String("body");
    writer.StartObject(); //body

    writer.String("currency");
    writer.String(request_credentials.request_body_currency_.c_str());

    writer.String("label");
    writer.String(request_credentials.request_body_label_.c_str());

    writer.String("publicKey");
    writer.String(request_credentials.request_body_publicKey_.c_str());

    writer.EndObject(); //body

    writer.String("octets");
    writer.String(request_credentials.request_body_octets_.c_str());

    writer.EndObject(); //request

    writer.String("proof");
    writer.String(request_credentials.proof_.c_str());
    writer.EndObject(); //root
    return buffer.GetString();
  }

  std::string stringifyReconcilePayloadSt(const RECONCILE_PAYLOAD_ST& reconcile_payload) {
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject(); //root

    writer.String("requestType");
    writer.String(reconcile_payload.requestType_.c_str());

    writer.String("signedTx");
    writer.StartObject(); //signedTx

    writer.String("headers");
    writer.StartObject(); //headers

    writer.String("digest");
    writer.String(reconcile_payload.request_signedtx_headers_digest_.c_str());

    writer.String("signature");
    writer.String(reconcile_payload.request_signedtx_headers_signature_.c_str());

    writer.EndObject(); //headers

    writer.String("body");
    writer.StartObject(); //body


    writer.String("denomination");
    writer.StartObject(); //denomination

    writer.String("amount");
    writer.String(reconcile_payload.request_signedtx_body_.amount_.c_str());

    writer.String("currency");
    writer.String(reconcile_payload.request_signedtx_body_.currency_.c_str());

    writer.EndObject(); //denomination

    writer.String("destination");
    writer.String(reconcile_payload.request_signedtx_body_.destination_.c_str());

    writer.EndObject(); //body

    writer.String("octets");
    writer.String(reconcile_payload.request_signedtx_octets_.c_str());

    writer.EndObject(); //signedTx

    writer.String("surveyorId");
    writer.String(reconcile_payload.request_surveyorId_.c_str());

    writer.String("viewingId");
    writer.String(reconcile_payload.request_viewingId_.c_str());

    writer.EndObject(); //root
    return buffer.GetString();
  }

  std::vector<uint8_t> getSHA256(const std::string& in) {
    std::vector<uint8_t> res(SHA256_DIGEST_LENGTH);
    SHA256((uint8_t*)in.c_str(), in.length(), &res.front());
    return res;
  }

  std::string getBase64(const std::vector<uint8_t>& in) {
    std::string res;
    size_t size = 0;
    if (!EVP_EncodedLength(&size, in.size())) {
      DCHECK(false);
      LOG(ERROR) << "EVP_EncodedLength failure in getBase64";

      return "";
    }
    std::vector<uint8_t> out(size);
    int numEncBytes = EVP_EncodeBlock(&out.front(), &in.front(), in.size());
    DCHECK(numEncBytes != 0);
    res = (char*)&out.front();
    return res;
  }

  bool getFromBase64(const std::string& in, std::vector<uint8_t> & out) {
    bool succeded = true;
    size_t size = 0;
    if (!EVP_DecodedLength(&size, in.length())) {
      DCHECK(false);
      LOG(ERROR) << "EVP_DecodedLength failure in getFromBase64";
      succeded = false;
    }

    if (succeded) {
      out.resize(size);
      size_t final_size = 0;
      int numDecBytes = EVP_DecodeBase64(&out.front(), &final_size, size, (const uint8_t*)in.c_str(), in.length());
      DCHECK(numDecBytes != 0);

      if (0 == numDecBytes) {
        succeded = false;
        out.clear();
      }
      else if (final_size != size)
      {
        out.resize(final_size);
      }
    }
    return succeded;
  }

  std::string sign(std::string* keys, std::string* values, const unsigned int& size,
      const std::string& keyId, const std::vector<uint8_t>& secretKey) {
    std::string headers;
    std::string message;
    for (unsigned int i = 0; i < size; i++) {
      if (0 != i) {
        headers += " ";
        message += "\n";
      }
      headers += keys[i];
      message += keys[i] + ": " + values[i];
    }
    std::vector<uint8_t> signedMsg(crypto_sign_BYTES + message.length());

    unsigned long long signedMsgSize = 0;
    crypto_sign(&signedMsg.front(), &signedMsgSize, (const unsigned char*)message.c_str(), (unsigned long long)message.length(), &secretKey.front());

    std::vector<uint8_t> signature(crypto_sign_BYTES);
    std::copy(signedMsg.begin(), signedMsg.begin() + crypto_sign_BYTES, signature.begin());

    return "keyId=\"" + keyId + "\",algorithm=\"" + SIGNATURE_ALGORITHM +
      "\",headers=\"" + headers + "\",signature=\"" + getBase64(signature) + "\"";
  }

  uint64_t currentTime() {
    return time(0);
  }

  void getTwitchParts(const std::string& query, std::vector<std::map<std::string, std::string>>& parts) {
    size_t pos = query.find("data=");
    if (std::string::npos != pos && query.length() > 5) {
      std::string varValue = query.substr(5);
      //DecodeURLChars(query.substr(5), varValue);
      std::vector<uint8_t> decoded;
      bool succeded = braveledger_bat_helper::getFromBase64(varValue, decoded);
      if (succeded) {
        decoded.push_back((uint8_t)'\0');
        braveledger_bat_helper::getJSONTwitchProperties((char*)&decoded.front(), parts);
      }
      else{
        LOG(ERROR) << "getTwitchParts failed in getFromBase64";
      }
    }
  }

  std::string getMediaId(const std::map<std::string, std::string>& data, const std::string& type) {
    if (YOUTUBE_MEDIA_TYPE == type) {
      std::map<std::string, std::string>::const_iterator iter = data.find("docid");
      if (iter != data.end()) {
        return iter->second;
      }
    } else if (TWITCH_MEDIA_TYPE == type) {
      std::map<std::string, std::string>::const_iterator iter = data.find("event");
      if (iter != data.end() && data.find("properties") != data.end()) {
        for (size_t i = 0; i < braveledger_ledger::_twitch_events_array_size; i++) {
          if (iter->second == braveledger_ledger::_twitch_events[i]) {
            iter = data.find("channel");
            std::string id("");
            if (iter != data.end()) {
              id = iter->second;
            }
            iter = data.find("vod");
            if (iter != data.end()) {
              std::string idAddition(iter->second);
              auto new_end = std::remove(idAddition.begin(), idAddition.end(), 'v');
              id += "_vod_" + idAddition;
              [&new_end] {}(); /*ignore [[nodiscard]]*/
            }

            return id;
          }
        }
      }
    }

    return "";
  }

  std::string getMediaKey(const std::string& mediaId, const std::string& type) {
    return type + "_" + mediaId;
  }

  uint64_t getMediaDuration(const std::map<std::string, std::string>& data, const std::string& media_key, const std::string& type) {
    uint64_t duration = 0;

    if (YOUTUBE_MEDIA_TYPE == type) {
      std::map<std::string, std::string>::const_iterator iterSt = data.find("st");
      std::map<std::string, std::string>::const_iterator iterEt = data.find("et");
      if (iterSt != data.end() && iterEt != data.end()) {
        std::vector<std::string> startTime;
        std::vector<std::string> endTime;
        split(startTime, iterSt->second, ',');
        split(endTime, iterEt->second, ',');
        if (startTime.size() != endTime.size()) {
          return 0;
        }
        double tempTime = 0;
        for (size_t i = 0; i < startTime.size(); i++) {
          std::stringstream tempET(endTime[i]);
          std::stringstream tempST(startTime[i]);
          double st = 0;
          double et = 0;
          tempET >> et;
          tempST >> st;
          tempTime = et - st;
        }
        duration = (uint64_t)tempTime;
      }
    } else if (TWITCH_MEDIA_TYPE == type) {
      // We set the correct duration for twitch in BatGetMedia class
      duration = 0;
    }

    return duration;
  }

  std::string buildURL(const std::string& path, const std::string& prefix, const SERVER_TYPES& server) {
    std::string url;
    switch (server) {
      case SERVER_TYPES::BALANCE:
        if (ledger::is_production) {
          url = BALANCE_PRODUCTION_SERVER;
        } else {
          url = BALANCE_STAGING_SERVER;
        }
        break;
      case SERVER_TYPES::PUBLISHER:
        if (ledger::is_production) {
          url = PUBLISHER_PRODUCTION_SERVER;
        } else {
          url = PUBLISHER_STAGING_SERVER;
        }
        break;
      default:
        if (ledger::is_production) {
          url = LEDGER_PRODUCTION_SERVER;
        } else {
          url = LEDGER_STAGING_SERVER;
        }
        break;
    }

    return url + prefix + path;
  }

  std::vector<std::string> split(const std::string& s, char delim) {
      std::stringstream ss(s);
      std::string item;
      std::vector<std::string> result;
      while (getline(ss, item, delim)) {
          result.push_back(item);
      }

      return result;
  }

  void set_ignore_for_testing(bool ignore) {
    ignore_ = ignore;
  }

  bool ignore_for_testing() {
    return ignore_;
  }

}  // namespace braveledger_bat_helper

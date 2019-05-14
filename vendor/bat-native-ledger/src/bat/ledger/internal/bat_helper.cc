/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <openssl/base64.h>
#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>
#include <iomanip>
#include <random>
#include <algorithm>
#include <utility>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "third_party/re2/src/re2/re2.h"
#include "tweetnacl.h"  // NOLINT
#include "url/gurl.h"

namespace braveledger_bat_helper {

bool isProbiValid(const std::string& probi) {
  // probi shouldn't be longer then 44
  if (probi.length() > 44) {
    return false;
  }

  // checks if probi only contains numbers
  return re2::RE2::FullMatch(probi, "^-?[0-9]*$");
}

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

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("paymentId") && d["paymentId"].IsString() &&
      d.HasMember("addressBAT") && d["addressBAT"].IsString() &&
      d.HasMember("addressBTC") && d["addressBTC"].IsString() &&
      d.HasMember("addressCARD_ID") && d["addressCARD_ID"].IsString() &&
      d.HasMember("addressETH") && d["addressETH"].IsString() &&
      d.HasMember("addressLTC") && d["addressLTC"].IsString() &&
      d.HasMember("keyInfoSeed") && d["keyInfoSeed"].IsString() );
  }

  if (!error) {
    // convert keyInfoSeed and check error
    std::string sKeyInfoSeed = d["keyInfoSeed"].GetString();
    error = !getFromBase64(sKeyInfoSeed, &keyInfoSeed_);
  }

  if (!error) {
    paymentId_ = d["paymentId"].GetString();
    addressBAT_ = d["addressBAT"].GetString();
    addressBTC_ = d["addressBTC"].GetString();
    addressCARD_ID_ = d["addressCARD_ID"].GetString();
    addressETH_ = d["addressETH"].GetString();
    addressLTC_ = d["addressLTC"].GetString();
  }
  return !error;
}

void saveToJson(JsonWriter* writer, const WALLET_INFO_ST& data) {
  writer->StartObject();

  writer->String("paymentId");
  writer->String(data.paymentId_.c_str());

  writer->String("addressBAT");
  writer->String(data.addressBAT_.c_str());

  writer->String("addressBTC");
  writer->String(data.addressBTC_.c_str());

  writer->String("addressCARD_ID");
  writer->String(data.addressCARD_ID_.c_str());

  writer->String("addressETH");
  writer->String(data.addressETH_.c_str());

  writer->String("addressLTC");
  writer->String(data.addressLTC_.c_str());

  writer->String("keyInfoSeed");
  if (data.keyInfoSeed_.size() == 0) {
    writer->String("");
  } else {
    writer->String(getBase64(data.keyInfoSeed_).c_str());
  }

  writer->EndObject();
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

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("publisher") && d["publisher"].IsString() &&
      d.HasMember("offset") && d["offset"].IsUint() );
  }

  if (!error) {
    publisher_ = d["publisher"].GetString();
    offset_ = d["offset"].GetUint();
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const TRANSACTION_BALLOT_ST& data) {
  writer->StartObject();

  writer->String("publisher");
  writer->String(data.publisher_.c_str());

  writer->String("offset");
  writer->Uint(data.offset_);

  writer->EndObject();
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
  votes_ = transaction.votes_;
  ballots_ = transaction.ballots_;
}

TRANSACTION_ST::~TRANSACTION_ST() {}

bool TRANSACTION_ST::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("viewingId") && d["viewingId"].IsString() &&
      d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
      d.HasMember("contribution_fiat_amount") &&
      d["contribution_fiat_amount"].IsString() &&
      d.HasMember("contribution_fiat_currency") &&
      d["contribution_fiat_currency"].IsString() &&
      d.HasMember("rates") && d["rates"].IsObject() &&
      d["rates"].HasMember("ETH") &&
      d["rates"].HasMember("LTC") &&
      d["rates"].HasMember("BTC") &&
      d["rates"].HasMember("USD") &&
      d["rates"].HasMember("EUR") &&
      d.HasMember("contribution_altcurrency") &&
      d["contribution_altcurrency"].IsString() &&
      d.HasMember("contribution_probi") && d["contribution_probi"].IsString() &&
      d.HasMember("contribution_fee") && d["contribution_fee"].IsString() &&
      d.HasMember("submissionStamp") && d["submissionStamp"].IsString() &&
      d.HasMember("submissionId") && d["submissionId"].IsString() &&
      d.HasMember("anonizeViewingId") && d["anonizeViewingId"].IsString() &&
      d.HasMember("registrarVK") && d["registrarVK"].IsString() &&
      d.HasMember("masterUserToken") && d["masterUserToken"].IsString() &&
      d.HasMember("surveyorIds") && d["surveyorIds"].IsArray() &&
      d.HasMember("votes") && d["votes"].IsUint() &&
      d.HasMember("ballots") && d["ballots"].IsArray());
  }

  if (!error) {
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
    votes_ = d["votes"].GetUint();

    for (auto & i : d["rates"].GetObject()) {
      contribution_rates_.insert(
          std::make_pair(i.name.GetString(), i.value.GetDouble()));
    }

    for (auto & i : d["surveyorIds"].GetArray()) {
      surveyorIds_.push_back(i.GetString());
    }

    for (const auto & i : d["ballots"].GetArray()) {
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


void saveToJson(JsonWriter* writer, const TRANSACTION_ST& data) {
  writer->StartObject();

  writer->String("viewingId");
  writer->String(data.viewingId_.c_str());

  writer->String("surveyorId");
  writer->String(data.surveyorId_.c_str());

  writer->String("contribution_fiat_amount");
  writer->String(data.contribution_fiat_amount_.c_str());

  writer->String("contribution_fiat_currency");
  writer->String(data.contribution_fiat_currency_.c_str());

  writer->String("rates");
  writer->StartObject();
  for (auto & p : data.contribution_rates_) {
    writer->String(p.first.c_str());
    writer->Double(p.second);
  }
  writer->EndObject();

  writer->String("contribution_altcurrency");
  writer->String(data.contribution_altcurrency_.c_str());

  writer->String("contribution_probi");
  writer->String(data.contribution_probi_.c_str());

  writer->String("contribution_fee");
  writer->String(data.contribution_fee_.c_str());

  writer->String("submissionStamp");
  writer->String(data.submissionStamp_.c_str());

  writer->String("submissionId");
  writer->String(data.submissionId_.c_str());

  writer->String("anonizeViewingId");
  writer->String(data.anonizeViewingId_.c_str());

  writer->String("registrarVK");
  writer->String(data.registrarVK_.c_str());

  writer->String("masterUserToken");
  writer->String(data.masterUserToken_.c_str());

  writer->String("surveyorIds");
  writer->StartArray();
  for (auto & i : data.surveyorIds_) {
    writer->String(i.c_str());
  }
  writer->EndArray();

  writer->String("votes");
  writer->Uint(data.votes_);

  writer->String("ballots");
  writer->StartArray();
  for (auto & i : data.ballots_) {
    saveToJson(writer, i);
  }
  writer->EndArray();

  writer->EndObject();
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

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("viewingId") &&  d["viewingId"].IsString() &&
      d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
      d.HasMember("publisher") && d["publisher"].IsString() &&
      d.HasMember("offset") && d["offset"].IsUint() &&
      d.HasMember("prepareBallot") && d["prepareBallot"].IsString() &&
      d.HasMember("delayStamp") && d["delayStamp"].IsUint64() );
  }

  if (!error) {
    viewingId_ = d["viewingId"].GetString();
    surveyorId_ = d["surveyorId"].GetString();
    publisher_ = d["publisher"].GetString();
    offset_ = d["offset"].GetUint();
    prepareBallot_ = d["prepareBallot"].GetString();
    delayStamp_ = d["delayStamp"].GetUint64();
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const BALLOT_ST& data) {
  writer->StartObject();

  writer->String("viewingId");
  writer->String(data.viewingId_.c_str());

  writer->String("surveyorId");
  writer->String(data.surveyorId_.c_str());

  writer->String("publisher");
  writer->String(data.publisher_.c_str());

  writer->String("offset");
  writer->Uint(data.offset_);

  writer->String("prepareBallot");
  writer->String(data.prepareBallot_.c_str());

  writer->String("delayStamp");
  writer->Uint64(data.delayStamp_);

  writer->EndObject();
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
  if (!error) {
    error = !(d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
      d.HasMember("proof") && d["proof"].IsString());
  }

  if (!error) {
    surveyorId_ = d["surveyorId"].GetString();
    proof_ = d["proof"].GetString();
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const BATCH_VOTES_INFO_ST& data) {
  writer->StartObject();

  writer->String("surveyorId");
  writer->String(data.surveyorId_.c_str());

  writer->String("proof");
  writer->String(data.proof_.c_str());

  writer->EndObject();
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
  if (!error) {
    error = !(d.HasMember("publisher") &&  d["publisher"].IsString() &&
      d.HasMember("batchVotesInfo") && d["batchVotesInfo"].IsArray());
  }

  if (!error) {
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

void saveToJson(JsonWriter* writer, const BATCH_VOTES_ST& data) {
  writer->StartObject();

  writer->String("publisher");
  writer->String(data.publisher_.c_str());

  writer->String("batchVotesInfo");
  writer->StartArray();
  for (auto & b : data.batchVotesInfo_) {
    saveToJson(writer, b);
  }
  writer->EndArray();

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
REPORT_BALANCE_ST::REPORT_BALANCE_ST():
  opening_balance_("0"),
  closing_balance_("0"),
  deposits_("0"),
  grants_("0"),
  earning_from_ads_("0"),
  auto_contribute_("0"),
  recurring_donation_("0"),
  one_time_donation_("0"),
  total_("0") {}

REPORT_BALANCE_ST::REPORT_BALANCE_ST(const REPORT_BALANCE_ST& state) {
  opening_balance_ = state.opening_balance_;
  closing_balance_ = state.closing_balance_;
  deposits_ = state.deposits_;
  grants_ = state.grants_;
  earning_from_ads_ = state.earning_from_ads_;
  auto_contribute_ = state.auto_contribute_;
  recurring_donation_ = state.recurring_donation_;
  one_time_donation_ = state.one_time_donation_;
  total_ = state.total_;
}

REPORT_BALANCE_ST::~REPORT_BALANCE_ST() {}

bool REPORT_BALANCE_ST::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("opening_balance") &&
        d["opening_balance"].IsString() &&
        isProbiValid(d["opening_balance"].GetString()) &&
        d.HasMember("closing_balance") && d["closing_balance"].IsString() &&
        isProbiValid(d["closing_balance"].GetString()) &&
        d.HasMember("deposits") && d["deposits"].IsString() &&
        isProbiValid(d["deposits"].GetString()) &&
        d.HasMember("grants") && d["grants"].IsString() &&
        isProbiValid(d["grants"].GetString()) &&
        d.HasMember("earning_from_ads") && d["earning_from_ads"].IsString() &&
        isProbiValid(d["earning_from_ads"].GetString()) &&
        d.HasMember("auto_contribute") && d["auto_contribute"].IsString() &&
        isProbiValid(d["auto_contribute"].GetString()) &&
        d.HasMember("recurring_donation") &&
        d["recurring_donation"].IsString() &&
        isProbiValid(d["recurring_donation"].GetString()) &&
        d.HasMember("one_time_donation") && d["one_time_donation"].IsString() &&
        isProbiValid(d["one_time_donation"].GetString()) &&
        d.HasMember("total") && d["total"].IsString() &&
        isProbiValid(d["total"].GetString()));
  }

  if (!error) {
    opening_balance_ = d["opening_balance"].GetString();
    closing_balance_ = d["closing_balance"].GetString();
    deposits_ = d["deposits"].GetString();
    grants_ = d["grants"].GetString();
    earning_from_ads_ = d["earning_from_ads"].GetString();
    auto_contribute_ = d["auto_contribute"].GetString();
    recurring_donation_ = d["recurring_donation"].GetString();
    one_time_donation_ = d["one_time_donation"].GetString();
    total_ = d["total"].GetString();
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const REPORT_BALANCE_ST& data) {
  writer->StartObject();

  writer->String("opening_balance");

  writer->String(data.opening_balance_.c_str());

  writer->String("closing_balance");

  writer->String(data.closing_balance_.c_str());

  writer->String("deposits");

  writer->String(data.deposits_.c_str());

  writer->String("grants");

  writer->String(data.grants_.c_str());

  writer->String("earning_from_ads");

  writer->String(data.earning_from_ads_.c_str());

  writer->String("auto_contribute");

  writer->String(data.auto_contribute_.c_str());

  writer->String("recurring_donation");

  writer->String(data.recurring_donation_.c_str());

  writer->String("one_time_donation");

  writer->String(data.one_time_donation_.c_str());

  writer->String("total");

  writer->String(data.total_.c_str());

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
PUBLISHER_STATE_ST::PUBLISHER_STATE_ST():
  min_publisher_duration_(braveledger_ledger::_default_min_publisher_duration),
  min_visits_(1),
  allow_non_verified_(true),
  pubs_load_timestamp_(0ull),
  allow_videos_(true) {}

PUBLISHER_STATE_ST::PUBLISHER_STATE_ST(const PUBLISHER_STATE_ST& state) {
  min_publisher_duration_ = state.min_publisher_duration_;
  min_visits_ = state.min_visits_;
  allow_non_verified_ = state.allow_non_verified_;
  pubs_load_timestamp_ = state.pubs_load_timestamp_;
  allow_videos_ = state.allow_videos_;
  monthly_balances_ = state.monthly_balances_;
  recurring_donation_ = state.recurring_donation_;
  migrate_score_2 = state.migrate_score_2;
}

PUBLISHER_STATE_ST::~PUBLISHER_STATE_ST() {}

bool PUBLISHER_STATE_ST::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("min_pubslisher_duration") &&
        d["min_pubslisher_duration"].IsUint() &&
        d.HasMember("min_visits") && d["min_visits"].IsUint() &&
        d.HasMember("allow_non_verified") && d["allow_non_verified"].IsBool() &&
        d.HasMember("pubs_load_timestamp") &&
        d["pubs_load_timestamp"].IsUint64() &&
        d.HasMember("allow_videos") && d["allow_videos"].IsBool() &&
        d.HasMember("monthly_balances") && d["monthly_balances"].IsArray() &&
        d.HasMember("recurring_donation") && d["recurring_donation"].IsArray());
  }

  if (!error) {
    min_publisher_duration_ = d["min_pubslisher_duration"].GetUint();
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
        recurring_donation_.insert(
            std::make_pair(itr->name.GetString(), itr->value.GetDouble()));
      }
    }

    if (d.HasMember("migrate_score_2") && d["migrate_score_2"].IsBool()) {
      migrate_score_2 = d["migrate_score_2"].GetBool();
    } else {
      migrate_score_2 = true;
    }
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const PUBLISHER_STATE_ST& data) {
  writer->StartObject();

  writer->String("min_pubslisher_duration");
  writer->Uint(data.min_publisher_duration_);

  writer->String("min_visits");
  writer->Uint(data.min_visits_);

  writer->String("allow_non_verified");
  writer->Bool(data.allow_non_verified_);

  writer->String("pubs_load_timestamp");
  writer->Uint64(data.pubs_load_timestamp_);

  writer->String("allow_videos");
  writer->Bool(data.allow_videos_);

  writer->String("monthly_balances");
  writer->StartArray();
  for (auto & p : data.monthly_balances_) {
    writer->StartObject();
    writer->String(p.first.c_str());
    saveToJson(writer, p.second);
    writer->EndObject();
  }
  writer->EndArray();

  writer->String("recurring_donation");
  writer->StartArray();
  for (auto & p : data.recurring_donation_) {
    writer->StartObject();
    writer->String(p.first.c_str());
    writer->Double(p.second);
    writer->EndObject();
  }

  writer->EndArray();

  writer->String("migrate_score_2");
  writer->Bool(data.migrate_score_2);

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
PUBLISHER_ST::PUBLISHER_ST():
  id_(""),
  duration_(0),
  score_(0),
  visits_(0),
  percent_(0),
  weight_(0),
  verified_(false) {}

PUBLISHER_ST::~PUBLISHER_ST() {}

bool PUBLISHER_ST::operator<(const PUBLISHER_ST& rhs) const {
  return score_ > rhs.score_;
}

bool PUBLISHER_ST::loadFromJson(const std::string& json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("id") && d["id"].IsString() &&
      d.HasMember("duration") && d["duration"].IsUint64() &&
      d.HasMember("score") && d["score"].IsDouble() &&
      d.HasMember("visits") && d["visits"].IsUint() &&
      d.HasMember("percent") && d["percent"].IsUint() &&
      d.HasMember("weight") && d["weight"].IsDouble());
  }

  if (!error) {
    id_ = d["id"].GetString();
    duration_ = d["duration"].GetUint64();
    score_ = d["score"].GetDouble();
    visits_ = d["visits"].GetUint();
    percent_ = d["percent"].GetUint();
    weight_ = d["pubs_load_timestamp"].GetDouble();

    if (d.HasMember("verified") && d["verified"].IsBool()) {
      verified_ = d["verified"].GetBool();
    } else {
      verified_ = false;
    }
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const PUBLISHER_ST& data) {
  writer->StartObject();

  writer->String("id");
  writer->String(data.id_.c_str());

  writer->String("duration");
  writer->Uint64(data.duration_);

  writer->String("score");
  writer->Double(data.score_);

  writer->String("visits");
  writer->Uint(data.visits_);

  writer->String("percent");
  writer->Uint(data.percent_);

  writer->String("weight");
  writer->Double(data.weight_);

  writer->String("verified");
  writer->Bool(data.verified_);

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
WINNERS_ST::WINNERS_ST() :
  votes_(0) {}

WINNERS_ST::~WINNERS_ST() {}

/////////////////////////////////////////////////////////////////////////////
WALLET_PROPERTIES_ST::WALLET_PROPERTIES_ST() :
  balance_(0), parameters_days_(0) {}

WALLET_PROPERTIES_ST::~WALLET_PROPERTIES_ST() {}

WALLET_PROPERTIES_ST::WALLET_PROPERTIES_ST(
    const WALLET_PROPERTIES_ST &properties) {
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

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(
      d.HasMember("altcurrency") && d["altcurrency"].IsString() &&
      d.HasMember("balance") && d["balance"].IsString() &&
      d.HasMember("probi") && d["probi"].IsString() &&
      d.HasMember("rates") && d["rates"].IsObject() &&
      d.HasMember("parameters") && d["parameters"].IsObject());
  }

  if (!error) {
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

        if (obj.HasMember("type")) {
          grant.type = obj["type"].GetString();
        }

        grants_.push_back(grant);
      }
    } else {
      grants_.clear();
    }
  }
  return !error;
}

void saveToJson(JsonWriter* writer, const WALLET_PROPERTIES_ST& data) {
  writer->StartObject();

  writer->String("altcurrency");
  writer->String(data.altcurrency_.c_str());

  writer->String("probi");
  writer->String(data.probi_.c_str());

  writer->String("balance");
  writer->String(std::to_string(data.balance_).c_str());

  writer->String("fee_amount");
  writer->Double(data.fee_amount_);

  writer->String("rates");
  writer->StartObject();
  for (auto & p : data.rates_) {
    writer->String(p.first.c_str());
    writer->Double(p.second);
  }
  writer->EndObject();

  writer->String("parameters");
  writer->StartObject();
  writer->String("adFree");
  writer->StartObject();

  writer->String("fee");
  writer->StartObject();
  writer->String("BAT");
  writer->Double(data.fee_amount_);
  writer->EndObject();


  writer->String("choices");
  writer->StartObject();
  writer->String("BAT");

  writer->StartArray();
  for (auto & choice : data.parameters_choices_) {
    writer->Double(choice);
  }
  writer->EndArray();
  writer->EndObject();


  writer->String("range");
  writer->StartObject();
  writer->String("BAT");

  writer->StartArray();
  for (const auto & range : data.parameters_range_) {
    writer->Double(range);
  }
  writer->EndArray();
  writer->EndObject();

  writer->String("days");
  writer->Int(data.parameters_days_);
  writer->EndObject();
  writer->EndObject();

  writer->String("grants");
  writer->StartArray();
  for (auto & grant : data.grants_) {
    saveToJson(writer, grant);
  }
  writer->EndArray();

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
GRANTS_PROPERTIES_ST::GRANTS_PROPERTIES_ST() {}

GRANTS_PROPERTIES_ST::GRANTS_PROPERTIES_ST(
    const GRANTS_PROPERTIES_ST& properties) {
  grants_ = properties.grants_;
}

GRANTS_PROPERTIES_ST::~GRANTS_PROPERTIES_ST() {}

bool GRANTS_PROPERTIES_ST::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("grants") && d["grants"].IsArray());
  }

  if (!error) {
    for (auto &i : d["grants"].GetArray()) {
      GRANT_RESPONSE grant;
      auto obj = i.GetObject();

      if (obj.HasMember("promotionId")) {
        grant.promotionId = obj["promotionId"].GetString();
      }

      if (obj.HasMember("minimumReconcileTimestamp")) {
        grant.minimumReconcileTimestamp =
            obj["minimumReconcileTimestamp"].GetUint64();
      }

      if (obj.HasMember("protocolVersion")) {
        grant.protocolVersion = obj["protocolVersion"].GetUint64();
      }

      if (obj.HasMember("type")) {
        grant.type = obj["type"].GetString();
      }

      grants_.push_back(grant);
    }
  } else {
    grants_.clear();
  }
  return !error;
}

void saveToJson(JsonWriter* writer, const GRANTS_PROPERTIES_ST& properties) {
  writer->StartObject();

  writer->String("grants");
  writer->StartArray();
  for (const auto& grant : properties.grants_) {
    writer->StartObject();

    writer->String("promotionId");
    writer->String(grant.promotionId.c_str());

    writer->String("minimumReconcileTimestamp");
    writer->Uint64(grant.minimumReconcileTimestamp);

    writer->String("protocolVersion");
    writer->Uint64(grant.protocolVersion);

    writer->String("type");
    writer->String(grant.type.c_str());

    writer->EndObject();
  }
  writer->EndArray();

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
GRANT::GRANT() : expiryTime(0) {}

GRANT::~GRANT() {}

GRANT::GRANT(const GRANT &properties) {
  promotionId = properties.promotionId;
  altcurrency = properties.altcurrency;
  expiryTime = properties.expiryTime;
  probi = properties.probi;
  type = properties.type;
}

bool GRANT::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (error) {
    return !error;
  }

  // First grant get
  error = !(
      d.HasMember("promotionId") && d["promotionId"].IsString());

  if (!error) {
    promotionId = d["promotionId"].GetString();
    return !error;
  }

  // On successful grant
  error = !(
      d.HasMember("altcurrency") && d["altcurrency"].IsString() &&
      d.HasMember("expiryTime") && d["expiryTime"].IsNumber() &&
      d.HasMember("probi") && d["probi"].IsString() &&
      d.HasMember("type") && d["type"].IsString());

  if (!error) {
    altcurrency = d["altcurrency"].GetString();
    expiryTime = d["expiryTime"].GetUint64();
    probi = d["probi"].GetString();
    type = d["type"].GetString();
  }

  return !error;
}

GRANT_RESPONSE::GRANT_RESPONSE() {}

GRANT_RESPONSE::~GRANT_RESPONSE() {}

GRANT_RESPONSE::GRANT_RESPONSE(const GRANT_RESPONSE &properties) {
  promotionId = properties.promotionId;
  minimumReconcileTimestamp = properties.minimumReconcileTimestamp;
  protocolVersion = properties.protocolVersion;
  type = properties.type;
}

bool GRANT_RESPONSE::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (error) {
    return !error;
  }

  // First grant get
  error = !(d.HasMember("promotionId") && d["promotionId"].IsString());

  if (!error) {
    promotionId = d["promotionId"].GetString();
    return !error;
  }

  // On successful grant
  error = !(
      d.HasMember("protocolVersion") && d["protocolVersion"].IsNumber() &&
      d.HasMember("minimumReconcileTimestamp") &&
      d["minimumReconcileTimestamp"].IsNumber() &&
      d.HasMember("type") && d["type"].IsString());

  if (!error) {
    minimumReconcileTimestamp = d["minimumReconcileTimestamp"].GetUint64();
    protocolVersion = d["protocolVersion"].GetUint64();
    type = d["type"].GetString();
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const GRANT& data) {
  writer->StartObject();

  writer->String("altcurrency");
  writer->String(data.altcurrency.c_str());

  writer->String("probi");
  writer->String(data.probi.c_str());

  writer->String("expiryTime");
  writer->Uint64(data.expiryTime);

  writer->String("promotionId");
  writer->String(data.promotionId.c_str());

  writer->String("type");
  writer->String(data.type.c_str());

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
SURVEYOR_INFO_ST::SURVEYOR_INFO_ST() {}

SURVEYOR_INFO_ST::~SURVEYOR_INFO_ST() {}

/////////////////////////////////////////////////////////////////////////////
SURVEYOR_ST::SURVEYOR_ST() {}

SURVEYOR_ST::~SURVEYOR_ST() {}

bool SURVEYOR_ST::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("signature") && d["signature"].IsString() &&
      d.HasMember("surveyorId") && d["surveyorId"].IsString() &&
      d.HasMember("surveyVK") && d["surveyVK"].IsString() &&
      d.HasMember("registrarVK") && d["registrarVK"].IsString());
  }

  if (!error) {
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

void saveToJson(JsonWriter* writer, const SURVEYOR_ST& data) {
  writer->StartObject();

  writer->String("signature");
  writer->String(data.signature_.c_str());

  writer->String("surveyorId");
  writer->String(data.surveyorId_.c_str());

  writer->String("surveyVK");
  writer->String(data.surveyVK_.c_str());

  writer->String("registrarVK");
  writer->String(data.registrarVK_.c_str());

  writer->String("surveySK");
  writer->String(data.surveySK_.c_str());

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
RECONCILE_DIRECTION::RECONCILE_DIRECTION() {}
RECONCILE_DIRECTION::RECONCILE_DIRECTION(const std::string& publisher_key,
                                         const int amount,
                                         const std::string& currency) :
  publisher_key_(publisher_key),
  amount_(amount),
  currency_(currency) {}
RECONCILE_DIRECTION::~RECONCILE_DIRECTION() {}

bool RECONCILE_DIRECTION::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("amount") && d["amount"].IsInt() &&
      d.HasMember("publisher_key") && d["publisher_key"].IsString() &&
      d.HasMember("currency") && d["currency"].IsString());
  }

  if (!error) {
    amount_ = d["amount"].GetInt();
    publisher_key_ = d["publisher_key"].GetString();
    currency_ = d["currency"].GetString();
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const RECONCILE_DIRECTION& data) {
  writer->StartObject();

  writer->String("amount");
  writer->Int(data.amount_);

  writer->String("publisher_key");
  writer->String(data.publisher_key_.c_str());

  writer->String("currency");
  writer->String(data.currency_.c_str());

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
CURRENT_RECONCILE::CURRENT_RECONCILE() :
  timestamp_(0),
  fee_(.0),
  retry_step_(ledger::ContributionRetry::STEP_NO),
  retry_level_(0) {}

CURRENT_RECONCILE::CURRENT_RECONCILE(const CURRENT_RECONCILE& data):
  viewingId_(data.viewingId_),
  anonizeViewingId_(data.anonizeViewingId_),
  registrarVK_(data.registrarVK_),
  preFlight_(data.preFlight_),
  masterUserToken_(data.masterUserToken_),
  surveyorInfo_(data.surveyorInfo_),
  timestamp_(data.timestamp_),
  rates_(data.rates_),
  amount_(data.amount_),
  currency_(data.currency_),
  fee_(data.fee_),
  directions_(data.directions_),
  category_(data.category_),
  list_(data.list_),
  retry_step_(data.retry_step_),
  retry_level_(data.retry_level_),
  destination_(data.destination_),
  proof_(data.proof_) {}

CURRENT_RECONCILE::~CURRENT_RECONCILE() {}

bool CURRENT_RECONCILE::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("viewingId") && d["viewingId"].IsString() &&
      d.HasMember("fee") && d["fee"].IsDouble() &&
      d.HasMember("category") && d["category"].IsInt());
  }

  if (!error) {
    viewingId_ = d["viewingId"].GetString();
    anonizeViewingId_ = d["anonizeViewingId"].GetString();
    registrarVK_ = d["registrarVK"].GetString();
    preFlight_ = d["preFlight"].GetString();
    masterUserToken_ = d["masterUserToken"].GetString();
    timestamp_ = d["timestamp"].GetUint64();
    amount_ = d["amount"].GetString();
    currency_ = d["currency"].GetString();
    fee_ = d["fee"].GetDouble();
    category_ = d["category"].GetInt();

    if (d.HasMember("surveyorInfo") && d["surveyorInfo"].IsObject()) {
      auto obj = d["surveyorInfo"].GetObject();
      SURVEYOR_INFO_ST info;
      info.surveyorId_ = obj["surveyorId"].GetString();
      surveyorInfo_ = info;
    }

    if (d.HasMember("rates") && d["rates"].IsObject()) {
      for (auto & i : d["rates"].GetObject()) {
        rates_.insert(std::make_pair(i.name.GetString(), i.value.GetDouble()));
      }
    }

    if (d.HasMember("directions") && d["directions"].IsArray()) {
      for (auto & i : d["directions"].GetArray()) {
        auto obj = i.GetObject();
        RECONCILE_DIRECTION direction;
        direction.amount_ = obj["amount"].GetInt();
        direction.publisher_key_ = obj["publisher_key"].GetString();
        direction.currency_ = obj["currency"].GetString();
        directions_.push_back(direction);
      }
    }

    if (d.HasMember("list") && d["list"].IsArray()) {
      for (auto &i : d["list"].GetArray()) {
        PUBLISHER_ST publisher_st;

        auto obj = i.GetObject();
        publisher_st.id_ = obj["id"].GetString();
        publisher_st.duration_ = obj["duration"].GetUint64();
        publisher_st.score_ = obj["score"].GetDouble();
        publisher_st.visits_ = obj["visits"].GetUint();
        publisher_st.percent_ = obj["percent"].GetUint();
        publisher_st.weight_ = obj["weight"].GetDouble();

        list_.push_back(publisher_st);
      }
    }

    if (d.HasMember("retry_step") && d["retry_step"].IsInt()) {
      retry_step_ = static_cast<ledger::ContributionRetry>(
          d["retry_step"].GetInt());
    } else {
      retry_step_ = ledger::ContributionRetry::STEP_NO;
    }

    if (d.HasMember("retry_level") && d["retry_level"].IsInt()) {
      retry_level_ = d["retry_level"].GetInt();
    } else {
      retry_level_ = 0;
    }

    if (d.HasMember("destination") && d["destination"].IsString()) {
      destination_ = d["destination"].GetString();
    }

    if (d.HasMember("proof") && d["proof"].IsString()) {
      proof_ = d["proof"].GetString();
    }
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const CURRENT_RECONCILE& data) {
  writer->StartObject();

  writer->String("viewingId");
  writer->String(data.viewingId_.c_str());

  writer->String("anonizeViewingId");
  writer->String(data.anonizeViewingId_.c_str());

  writer->String("registrarVK");
  writer->String(data.registrarVK_.c_str());

  writer->String("preFlight");
  writer->String(data.preFlight_.c_str());

  writer->String("masterUserToken");
  writer->String(data.masterUserToken_.c_str());

  writer->String("surveyorInfo");
  writer->StartObject();
    writer->String("surveyorId");
    writer->String(data.surveyorInfo_.surveyorId_.c_str());
  writer->EndObject();

  writer->String("timestamp");
  writer->Uint64(data.timestamp_);

  writer->String("amount");
  writer->String(data.amount_.c_str());

  writer->String("currency");
  writer->String(data.currency_.c_str());

  writer->String("fee");
  writer->Double(data.fee_);

  writer->String("category");
  writer->Int(data.category_);

  writer->String("rates");
  writer->StartObject();
  for (auto & p : data.rates_) {
    writer->String(p.first.c_str());
    writer->Double(p.second);
  }
  writer->EndObject();

  writer->String("directions");
  writer->StartArray();
  for (auto & i : data.directions_) {
    saveToJson(writer, i);
  }
  writer->EndArray();

  writer->String("list");
  writer->StartArray();
  for (auto & i : data.list_) {
    saveToJson(writer, i);
  }
  writer->EndArray();

  writer->String("retry_step");
  writer->Int(data.retry_step_);

  writer->String("retry_level");
  writer->Int(data.retry_level_);

  writer->String("destination");
  writer->String(data.destination_.c_str());

  writer->String("proof");
  writer->String(data.proof_.c_str());

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
CLIENT_STATE_ST::CLIENT_STATE_ST():
  bootStamp_(0),
  reconcileStamp_(0),
  last_grant_fetch_stamp_(0),
  settings_(AD_FREE_SETTINGS),
  fee_amount_(0),
  user_changed_fee_(false),
  days_(0),
  auto_contribute_(false),
  rewards_enabled_(false) {}

CLIENT_STATE_ST::CLIENT_STATE_ST(const CLIENT_STATE_ST& other) {
  walletProperties_ = other.walletProperties_;
  walletInfo_ = other.walletInfo_;
  bootStamp_ = other.bootStamp_;
  reconcileStamp_ = other.reconcileStamp_;
  last_grant_fetch_stamp_ = other.last_grant_fetch_stamp_;
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
  current_reconciles_ = other.current_reconciles_;
  inline_tip_ = other.inline_tip_;
}

CLIENT_STATE_ST::~CLIENT_STATE_ST() {}

bool CLIENT_STATE_ST::loadFromJson(const std::string & json) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser error or wrong types
  bool error = d.HasParseError();
  if (!error) {
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
      d.HasMember("rewards_enabled") && d["rewards_enabled"].IsBool());
  }

  if (!error) {
    {
      auto & i = d["walletInfo"];
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      i.Accept(writer);
      walletInfo_.loadFromJson(sb.GetString());
    }

    bootStamp_ = d["bootStamp"].GetUint64();
    reconcileStamp_ = d["reconcileStamp"].GetUint64();

    if (d.HasMember("last_grant_fetch_stamp") &&
        d["last_grant_fetch_stamp"].IsUint64()) {
      last_grant_fetch_stamp_ = d["last_grant_fetch_stamp"].GetUint64();
    } else {
      last_grant_fetch_stamp_ = 0u;
    }

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

    if (d.HasMember("current_reconciles") &&
        d["current_reconciles"].IsObject()) {
      for (const auto & i : d["current_reconciles"].GetObject()) {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.value.Accept(writer);

        CURRENT_RECONCILE b;
        b.loadFromJson(sb.GetString());
        current_reconciles_[i.name.GetString()] = b;
      }
    }

    if (d.HasMember("walletProperties") && d["walletProperties"].IsObject()) {
      auto & i = d["walletProperties"];
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      i.Accept(writer);
      walletProperties_.loadFromJson(sb.GetString());
    }

    if (d.HasMember("inlineTip") && d["inlineTip"].IsObject()) {
      for (auto & k : d["inlineTip"].GetObject()) {
        inline_tip_.insert(
            std::make_pair(k.name.GetString(), k.value.GetBool()));
      }
    }
  }

  return !error;
}

void saveToJson(JsonWriter* writer, const CLIENT_STATE_ST& data) {
  writer->StartObject();

  writer->String("walletInfo");
  saveToJson(writer, data.walletInfo_);

  writer->String("bootStamp");
  writer->Uint64(data.bootStamp_);

  writer->String("reconcileStamp");
  writer->Uint64(data.reconcileStamp_);

  writer->String("last_grant_fetch_stamp");
  writer->Uint64(data.last_grant_fetch_stamp_);

  writer->String("personaId");
  writer->String(data.personaId_.c_str());

  writer->String("userId");
  writer->String(data.userId_.c_str());

  writer->String("registrarVK");
  writer->String(data.registrarVK_.c_str());

  writer->String("masterUserToken");
  writer->String(data.masterUserToken_.c_str());

  writer->String("preFlight");
  writer->String(data.preFlight_.c_str());

  writer->String("fee_currency");
  writer->String(data.fee_currency_.c_str());

  writer->String("settings");
  writer->String(data.settings_.c_str());

  writer->String("fee_amount");
  writer->Double(data.fee_amount_);

  writer->String("user_changed_fee");
  writer->Bool(data.user_changed_fee_);

  writer->String("days");
  writer->Uint(data.days_);

  writer->String("rewards_enabled");
  writer->Bool(data.rewards_enabled_);

  writer->String("auto_contribute");
  writer->Bool(data.auto_contribute_);

  writer->String("transactions");
  writer->StartArray();
  for (auto & t : data.transactions_) {
    saveToJson(writer, t);
  }
  writer->EndArray();

  writer->String("ballots");
  writer->StartArray();
  for (auto & b : data.ballots_) {
    saveToJson(writer, b);
  }
  writer->EndArray();

  writer->String("ruleset");
  writer->String(data.ruleset_.c_str());

  writer->String("rulesetV2");
  writer->String(data.rulesetV2_.c_str());

  writer->String("batch");
  writer->StartArray();
  for (auto & b : data.batch_) {
    saveToJson(writer, b);
  }
  writer->EndArray();

  writer->String("current_reconciles");
  writer->StartObject();
  for (auto & t : data.current_reconciles_) {
    writer->Key(t.first.c_str());
    saveToJson(writer, t.second);
  }
  writer->EndObject();

  writer->String("walletProperties");
  saveToJson(writer, data.walletProperties_);

  writer->String("inlineTip");
  writer->StartObject();
  for (auto & p : data.inline_tip_) {
    writer->String(p.first.c_str());
    writer->Bool(p.second);
  }
  writer->EndObject();

  writer->EndObject();
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

MEDIA_PUBLISHER_INFO::MEDIA_PUBLISHER_INFO(
    const MEDIA_PUBLISHER_INFO& mediaPublisherInfo):
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

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("publisherName") && d["publisherName"].IsString() &&
      d.HasMember("publisherURL") && d["publisherURL"].IsString() &&
      d.HasMember("favIconURL") && d["favIconURL"].IsString() &&
      d.HasMember("channelName") && d["channelName"].IsString() &&
      d.HasMember("publisherId") && d["publisherId"].IsString() &&
      d.HasMember("twitch_event") && d["twitch_event"].IsString() &&
      d.HasMember("twitch_time") && d["twitch_time"].IsString() &&
      d.HasMember("twitch_status") && d["twitch_status"].IsString());
  }

  if (!error) {
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

void saveToJson(JsonWriter* writer, const MEDIA_PUBLISHER_INFO& data) {
  writer->StartObject();

  writer->String("publisherName");
  writer->String(data.publisherName_.c_str());

  writer->String("publisherURL");
  writer->String(data.publisherURL_.c_str());

  writer->String("favIconURL");
  writer->String(data.favIconURL_.c_str());

  writer->String("channelName");
  writer->String(data.channelName_.c_str());

  writer->String("publisherId");
  writer->String(data.publisher_id_.c_str());

  writer->String("twitch_event");
  writer->String(data.twitchEventInfo_.event_.c_str());

  writer->String("twitch_time");
  writer->String(data.twitchEventInfo_.time_.c_str());

  writer->String("twitch_status");
  writer->String(data.twitchEventInfo_.status_.c_str());

  writer->EndObject();
}

/////////////////////////////////////////////////////////////////////////////
BATCH_PROOF::BATCH_PROOF() {}

BATCH_PROOF::~BATCH_PROOF() {}

/////////////////////////////////////////////////////////////////////////////
SERVER_LIST_BANNER::SERVER_LIST_BANNER() {}

SERVER_LIST_BANNER::SERVER_LIST_BANNER(const SERVER_LIST_BANNER& banner):
  title_(banner.title_),
  description_(banner.description_),
  background_(banner.background_),
  logo_(banner.logo_),
  amounts_(banner.amounts_),
  social_(banner.social_) {}

SERVER_LIST_BANNER::~SERVER_LIST_BANNER() {}

/////////////////////////////////////////////////////////////////////////////

bool getJSONValue(const std::string& fieldName,
                  const std::string& json,
                  std::string* value) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError() || (false == d.HasMember(fieldName.c_str()));
  if (!error) {
    *value = d[fieldName.c_str()].GetString();
  }
  return !error;
}

bool getJSONList(const std::string& fieldName,
                 const std::string& json,
                 std::vector<std::string>* value) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError() ||
      (false == (d.HasMember(fieldName.c_str()) &&
       d[fieldName.c_str()].IsArray()));
  if (!error) {
    for (auto & i : d[fieldName.c_str()].GetArray()) {
      value->push_back(i.GetString());
    }
  }
  return !error;
}

bool getJSONTwitchProperties(
    const std::string& json,
    std::vector<std::map<std::string, std::string>>* parts) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
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
      parts->push_back(eventmap);
    }
  }
  return !error;
}

bool getJSONBatchSurveyors(const std::string& json,
                           std::vector<std::string>* surveyors) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    for (auto & i : d.GetArray()) {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      i.Accept(writer);
      std::string surveyor = sb.GetString();
      surveyors->push_back(surveyor);
    }
  }

  return !error;
}

bool getJSONRates(const std::string& json,
                  std::map<std::string, double>* rates) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("rates") && d["rates"].IsObject() &&
      d["rates"].HasMember("ETH") &&
      d["rates"].HasMember("LTC") &&
      d["rates"].HasMember("BTC") &&
      d["rates"].HasMember("USD") &&
      d["rates"].HasMember("EUR"));
  }

  if (!error) {
    for (auto & i : d["rates"].GetObject()) {
      double value = 0.0;
      if (i.value.IsDouble()) {
        value = i.value.GetDouble();
      } else if (i.value.IsString()) {
        value = std::stod(i.value.GetString());
      }
      rates->insert(std::make_pair(i.name.GetString(), value));
    }
  }
  return !error;
}

bool getJSONTransaction(const std::string& json, TRANSACTION_ST* transaction) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("paymentStamp") && d["paymentStamp"].IsUint64() &&
      d.HasMember("probi") && d["probi"].IsString() &&
      d.HasMember("altcurrency") && d["altcurrency"].IsString() );
  }

  if (!error) {
    uint64_t stamp = d["paymentStamp"].GetUint64();
    transaction->submissionStamp_ = std::to_string(stamp);
    transaction->contribution_probi_ = d["probi"].GetString();
    transaction->contribution_altcurrency_ = d["altcurrency"].GetString();
  }
  return !error;
}

bool getJSONUnsignedTx(const std::string& json, UNSIGNED_TX* unsignedTx) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !d.HasMember("unsignedTx") || !(d["unsignedTx"].IsObject());
  }

  if (!error) {
    unsignedTx->amount_ =
        d["unsignedTx"]["denomination"]["amount"].GetString();
    unsignedTx->currency_ =
        d["unsignedTx"]["denomination"]["currency"].GetString();
    unsignedTx->destination_ = d["unsignedTx"]["destination"].GetString();
  }
  return !error;
}

bool getJSONWalletInfo(const std::string& json,
                       WALLET_INFO_ST* walletInfo,
                       std::string* fee_currency,
                       double* fee_amount,
                       unsigned int* days) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(
        ((d.HasMember("parameters") && d["parameters"].IsObject()) &&
         (d.HasMember("addresses") && d["addresses"].IsObject())) ||
        ((d.HasMember("payload") && d["payload"].IsObject()) &&
         (d.HasMember("wallet") && d["wallet"].IsObject())));
  }

  if (!error) {
    if (d.HasMember("payload") && d["payload"].IsObject()) {
      walletInfo->paymentId_ = d["wallet"]["paymentId"].GetString();
      walletInfo->addressBAT_ = d["wallet"]["addresses"]["BAT"].GetString();
      walletInfo->addressBTC_ = d["wallet"]["addresses"]["BTC"].GetString();
      walletInfo->addressCARD_ID_ =
          d["wallet"]["addresses"]["CARD_ID"].GetString();
      walletInfo->addressETH_ = d["wallet"]["addresses"]["ETH"].GetString();
      walletInfo->addressLTC_ = d["wallet"]["addresses"]["LTC"].GetString();

      *days = d["payload"]["adFree"]["days"].GetUint();
      const auto & fee = d["payload"]["adFree"]["fee"].GetObject();
      auto itr = fee.MemberBegin();
      if (itr != fee.MemberEnd()) {
        *fee_currency = itr->name.GetString();
        *fee_amount = itr->value.GetDouble();
      }
    } else if (d.HasMember("parameters") && d["parameters"].IsObject()) {
      walletInfo->addressBAT_ = d["addresses"]["BAT"].GetString();
      walletInfo->addressBTC_ = d["addresses"]["BTC"].GetString();
      walletInfo->addressCARD_ID_ = d["addresses"]["CARD_ID"].GetString();
      walletInfo->addressETH_ = d["addresses"]["ETH"].GetString();
      walletInfo->addressLTC_ = d["addresses"]["LTC"].GetString();
      *days = d["parameters"]["adFree"]["days"].GetUint();
      const auto & fee = d["parameters"]["adFree"]["fee"].GetObject();
      auto itr = fee.MemberBegin();
      if (itr != fee.MemberEnd()) {
        *fee_currency = itr->name.GetString();
        *fee_amount = itr->value.GetDouble();
      }
    }
  }
  return !error;
}

bool getJSONRecoverWallet(const std::string& json,
                          double* balance,
                          std::string* probi,
                          std::vector<GRANT>* grants) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("balance") && d["balance"].IsString() &&
              d.HasMember("probi") && d["probi"].IsString() );
  }

  if (!error) {
    *balance = std::stod(d["balance"].GetString());
    *probi = d["probi"].GetString();

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

        if (obj.HasMember("type")) {
          grant.type = obj["type"].GetString();
        }

        grants->push_back(grant);
      }
    } else {
      grants->clear();
    }
  }
  return !error;
}

bool getJSONResponse(const std::string& json,
                     unsigned int* statusCode,
                     std::string* error) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool hasError = d.HasParseError();
  if (!hasError) {
    hasError = !(d.HasMember("statusCode") && d["statusCode"].IsNumber() &&
              d.HasMember("error") && d["error"].IsString());
  }

  if (!hasError) {
    *statusCode = d["statusCode"].GetUint();
    *error = d["error"].GetString();
  }
  return !hasError;
}

bool getJSONGrant(const std::string& json, uint64_t* expiryTime) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool hasError = d.HasParseError();
  if (!hasError) {
    hasError = !(d.HasMember("expiryTime") && d["expiryTime"].IsNumber());
  }

  if (!hasError) {
    *expiryTime = d["expiryTime"].GetUint();
  }
  return !hasError;
}

bool getJSONServerList(const std::string& json,
                       std::map<std::string, SERVER_LIST>* list) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  bool hasError = d.HasParseError();
  if (!hasError) {
    hasError = !d.IsArray();
  }

  *list = {};

  if (!hasError) {
    for (auto &i : d.GetArray()) {
      SERVER_LIST item;
      item.verified = i[1].GetBool();
      item.excluded = i[2].GetBool();

      SERVER_LIST_BANNER banner;

      if (i.Size() > 3 && i[3].IsObject()) {
        if (i[3].HasMember("title") && i[3]["title"].IsString()) {
          banner.title_ = i[3]["title"].GetString();
        }

        if (i[3].HasMember("description") && i[3]["description"].IsString()) {
          banner.description_ = i[3]["description"].GetString();
        }

        if (i[3].HasMember("backgroundUrl") &&
            i[3]["backgroundUrl"].IsString()) {
          banner.background_ = i[3]["backgroundUrl"].GetString();
        }

        if (i[3].HasMember("logoUrl") && i[3]["logoUrl"].IsString()) {
          banner.logo_ = i[3]["logoUrl"].GetString();
        }

        if (i[3].HasMember("donationAmounts") &&
            i[3]["donationAmounts"].IsArray()) {
          for (auto &j : i[3]["donationAmounts"].GetArray()) {
            banner.amounts_.emplace_back(j.GetInt());
          }
        }

        if (i[3].HasMember("socialLinks") && i[3]["socialLinks"].IsObject()) {
          for (auto & k : i[3]["socialLinks"].GetObject()) {
            banner.social_.insert(
                std::make_pair(k.name.GetString(), k.value.GetString()));
          }
        }
      }

      item.banner = banner;

      list->emplace(i[0].GetString(), item);
    }
  }

  return !hasError;
}

bool getJSONAddresses(const std::string& json,
                      std::map<std::string, std::string>* addresses) {
  rapidjson::Document d;
  d.Parse(json.c_str());

  // has parser errors or wrong types
  bool error = d.HasParseError();
  if (!error) {
    error = !(d.HasMember("addresses") && d["addresses"].IsObject());
  }

  if (!error) {
    addresses->insert(
        std::make_pair("BAT", d["addresses"]["BAT"].GetString()));
    addresses->insert(
        std::make_pair("BTC", d["addresses"]["BTC"].GetString()));
    addresses->insert(
        std::make_pair("CARD_ID", d["addresses"]["CARD_ID"].GetString()));
    addresses->insert(
        std::make_pair("ETH", d["addresses"]["ETH"].GetString()));
    addresses->insert(
        std::make_pair("LTC", d["addresses"]["LTC"].GetString()));
  }

  return !error;
}

std::vector<uint8_t> generateSeed() {
  std::vector<uint8_t> vSeed(SEED_LENGTH);
  std::random_device r;
  std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
  auto rand = std::bind(std::uniform_int_distribution<>(0, UCHAR_MAX),
                        std::mt19937(seed));

  std::generate_n(vSeed.begin(), SEED_LENGTH, rand);
  return vSeed;
}

std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed) {
  DCHECK(!seed.empty());
  std::vector<uint8_t> out(SEED_LENGTH);

  const uint8_t info[] = {0};
  int hkdfRes = HKDF(&out.front(),
                     SEED_LENGTH,
                     EVP_sha512(),
                     &seed.front(),
                     seed.size(),
                     braveledger_ledger::g_hkdfSalt,
                     SALT_LENGTH,
                     info,
                     sizeof(info) / sizeof(info[0]));

  DCHECK(hkdfRes);
  DCHECK(!seed.empty());

  // We set the key_length to the length of the expected output and then take
  // the result from the first key, which is the client write key.

  return out;
}

bool getPublicKeyFromSeed(const std::vector<uint8_t>& seed,
                          std::vector<uint8_t>* publicKey,
                          std::vector<uint8_t>* secretKey) {
  DCHECK(!seed.empty());
  if (seed.empty()) {
    return false;
  }
  publicKey->resize(crypto_sign_PUBLICKEYBYTES);
  *secretKey = seed;
  secretKey->resize(crypto_sign_SECRETKEYBYTES);

  crypto_sign_keypair(&publicKey->front(), &secretKey->front(), 1);

  DCHECK(!publicKey->empty() && !secretKey->empty());
  if (publicKey->empty() && secretKey->empty()) {
    return false;
  }

  return true;
}

std::string uint8ToHex(const std::vector<uint8_t>& in) {
  std::ostringstream res;
  for (size_t i = 0; i < in.size(); i++) {
    res << std::setfill('0') << std::setw(sizeof(uint8_t) * 2)
       << std::hex << static_cast<int>(in[i]);
  }
  return res.str();
}


std::string stringifyBatch(std::vector<BATCH_VOTES_INFO_ST> payload) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  writer.StartArray();
  for (auto & d : payload) {
    saveToJson(&writer, d);
  }
  writer.EndArray();

  return buffer.GetString();
}

std::string stringify(std::string* keys,
                      std::string* values,
                      const unsigned int size) {
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

std::string stringifyRequestCredentialsSt(
    const REQUEST_CREDENTIALS_ST& request_credentials) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);
  writer.StartObject();  // root

  writer.String("requestType");
  writer.String(request_credentials.requestType_.c_str());

  writer.String("request");
  writer.StartObject();  // request

  writer.String("headers");
  writer.StartObject();  // headers

  writer.String("digest");
  writer.String(request_credentials.request_headers_digest_.c_str());

  writer.String("signature");
  writer.String(request_credentials.request_headers_signature_.c_str());

  writer.EndObject();  // headers

  writer.String("body");
  writer.StartObject();  // body

  writer.String("currency");
  writer.String(request_credentials.request_body_currency_.c_str());

  writer.String("label");
  writer.String(request_credentials.request_body_label_.c_str());

  writer.String("publicKey");
  writer.String(request_credentials.request_body_publicKey_.c_str());

  writer.EndObject();  // body

  writer.String("octets");
  writer.String(request_credentials.request_body_octets_.c_str());

  writer.EndObject();  // request

  writer.String("proof");
  writer.String(request_credentials.proof_.c_str());
  writer.EndObject();  // root
  return buffer.GetString();
}

std::string stringifyReconcilePayloadSt(
    const RECONCILE_PAYLOAD_ST& reconcile_payload) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);
  writer.StartObject();  // root

  writer.String("requestType");
  writer.String(reconcile_payload.requestType_.c_str());

  writer.String("signedTx");
  writer.StartObject();  // signedTx

  writer.String("headers");
  writer.StartObject();  // headers

  writer.String("digest");
  writer.String(reconcile_payload.request_signedtx_headers_digest_.c_str());

  writer.String("signature");
  writer.String(reconcile_payload.request_signedtx_headers_signature_.c_str());

  writer.EndObject();  // headers

  writer.String("body");
  writer.StartObject();  // body


  writer.String("denomination");
  writer.StartObject();  // denomination

  writer.String("amount");
  writer.String(reconcile_payload.request_signedtx_body_.amount_.c_str());

  writer.String("currency");
  writer.String(reconcile_payload.request_signedtx_body_.currency_.c_str());

  writer.EndObject();  // denomination

  writer.String("destination");
  writer.String(reconcile_payload.request_signedtx_body_.destination_.c_str());

  writer.EndObject();  // body

  writer.String("octets");
  writer.String(reconcile_payload.request_signedtx_octets_.c_str());

  writer.EndObject();  // signedTx

  writer.String("surveyorId");
  writer.String(reconcile_payload.request_surveyorId_.c_str());

  writer.String("viewingId");
  writer.String(reconcile_payload.request_viewingId_.c_str());

  writer.EndObject();  // root
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
    return "";
  }
  std::vector<uint8_t> out(size);
  int numEncBytes = EVP_EncodeBlock(&out.front(), &in.front(), in.size());
  DCHECK_NE(numEncBytes, 0);
  res = reinterpret_cast<char*>(&out.front());
  return res;
}

bool getFromBase64(const std::string& in, std::vector<uint8_t>* out) {
  bool succeded = true;
  size_t size = 0;
  if (!EVP_DecodedLength(&size, in.length())) {
    DCHECK(false);
    succeded = false;
  }

  if (succeded) {
    out->resize(size);
    size_t final_size = 0;
    int numDecBytes = EVP_DecodeBase64(&out->front(),
                                       &final_size,
                                       size,
                                       (const uint8_t*)in.c_str(),
                                       in.length());
    DCHECK_NE(numDecBytes, 0);

    if (numDecBytes == 0) {
      succeded = false;
      out->clear();
    } else if (final_size != size) {
      out->resize(final_size);
    }
  }
  return succeded;
}

std::string sign(std::string* keys,
                 std::string* values,
                 const unsigned int size,
                 const std::string& keyId,
                 const std::vector<uint8_t>& secretKey) {
  std::string headers;
  std::string message;
  for (unsigned int i = 0; i < size; i++) {
    if (i != 0) {
      headers += " ";
      message += "\n";
    }
    headers += keys[i];
    message += keys[i] + ": " + values[i];
  }
  std::vector<uint8_t> signedMsg(crypto_sign_BYTES + message.length());

  unsigned long long signedMsgSize = 0;  // NOLINT
  crypto_sign(&signedMsg.front(),
              &signedMsgSize,
              reinterpret_cast<const unsigned char*>(message.c_str()),
              (unsigned long long)message.length(),  // NOLINT
              &secretKey.front());

  std::vector<uint8_t> signature(crypto_sign_BYTES);
  std::copy(signedMsg.begin(),
            signedMsg.begin() + crypto_sign_BYTES,
            signature.begin());

  return "keyId=\"" + keyId + "\",algorithm=\"" + SIGNATURE_ALGORITHM +
    "\",headers=\"" + headers + "\",signature=\"" + getBase64(signature) + "\"";
}

uint64_t currentTime() {
  return time(0);
}

bool HasSameDomainAndPath(
    const std::string& url_to_validate,
    const std::string& domain_to_match,
    const std::string& path_to_match) {
  GURL gurl(url_to_validate);
  return gurl.is_valid() && !domain_to_match.empty() &&
      gurl.DomainIs(domain_to_match) &&
      gurl.has_path() && !path_to_match.empty() &&
      gurl.path().substr(0, path_to_match.size()) == path_to_match;
}

std::string buildURL(const std::string& path,
                     const std::string& prefix,
                     const SERVER_TYPES& server) {
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
    case SERVER_TYPES::PUBLISHER_DISTRO:
      if (ledger::is_production) {
        url = PUBLISHER_DISTRO_PRODUCTION_SERVER;
      } else {
        url = PUBLISHER_DISTRO_STAGING_SERVER;
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
    if (s[0] != '\n') {
      result.push_back(item);
    }
  }

  return result;
}

bool ignore_for_testing() {
  return ledger::is_testing;
}

std::string toLowerCase(std::string word) {
  std::transform(word.begin(), word.end(), word.begin(), ::tolower);
  return word;
}

uint8_t niceware_mnemonic_to_bytes(
    const std::string& w,
    std::vector<uint8_t>* bytes_out,
    size_t* written,
    std::vector<std::string> wordDictionary) {
  std::vector<std::string> wordList = braveledger_bat_helper::split(
      toLowerCase(w),
      WALLET_PASSPHRASE_DELIM);
  std::vector<uint8_t> buffer(wordList.size() * 2);

  for (uint8_t ix = 0; ix < wordList.size(); ix++) {
    std::vector<std::string>::iterator it =
      std::find(wordDictionary.begin(),
      wordDictionary.end(), wordList[ix]);
    if (it != wordDictionary.end()) {
      int wordIndex = std::distance(wordDictionary.begin(), it);
      buffer[2 * ix] = floor(wordIndex / 256);
      buffer[2 * ix + 1] = wordIndex % 256;
    } else {
      return INVALID_LEGACY_WALLET;
    }
  }
  *bytes_out = buffer;
  *written = NICEWARE_BYTES_WRITTEN;
  return 0;
}

void saveToJson(JsonWriter* writer, const ledger::VisitData& visitData) {
  writer->StartObject();

  writer->String("tld");
  writer->String(visitData.tld.c_str());

  writer->String("domain");
  writer->String(visitData.domain.c_str());

  writer->String("path");
  writer->String(visitData.path.c_str());

  writer->String("tab_id");
  writer->Uint(visitData.tab_id);

  writer->String("name");
  writer->String(visitData.name.c_str());

  writer->String("url");
  writer->String(visitData.url.c_str());

  writer->String("provider");
  writer->String(visitData.provider.c_str());

  writer->String("favicon_url");
  writer->String(visitData.favicon_url.c_str());

  writer->EndObject();
}

void saveToJson(JsonWriter* writer, const ledger::BalanceReportInfo& info) {
  writer->StartObject();

  writer->String("opening_balance_");
  writer->String(info.opening_balance_.c_str());

  writer->String("closing_balance_");
  writer->String(info.closing_balance_.c_str());

  writer->String("deposits_");
  writer->String(info.deposits_.c_str());

  writer->String("grants_");
  writer->String(info.grants_.c_str());

  writer->String("earning_from_ads_");
  writer->String(info.earning_from_ads_.c_str());

  writer->String("auto_contribute_");
  writer->String(info.auto_contribute_.c_str());

  writer->String("recurring_donation_");
  writer->String(info.recurring_donation_.c_str());

  writer->String("one_time_donation_");
  writer->String(info.one_time_donation_.c_str());

  writer->String("total_");
  writer->String(info.total_.c_str());

  writer->EndObject();
}

void saveToJson(JsonWriter* writer, const ledger::Grant& grant) {
  writer->StartObject();

  writer->String("altcurrency");
  writer->String(grant.altcurrency.c_str());

  writer->String("probi");
  writer->String(grant.probi.c_str());

  writer->String("promotionId");
  writer->String(grant.promotionId.c_str());

  writer->String("expiryTime");
  writer->Uint64(grant.expiryTime);

  writer->String("type");
  writer->String(grant.type.c_str());

  writer->EndObject();
}

void saveToJson(JsonWriter* writer,
    const ledger::PublisherBanner& banner) {
  writer->StartObject();

  writer->String("publisher_key");
  writer->String(banner.publisher_key.c_str());

  writer->String("title");
  writer->String(banner.title.c_str());

  writer->String("name");
  writer->String(banner.name.c_str());

  writer->String("description");
  writer->String(banner.description.c_str());

  writer->String("background");
  writer->String(banner.background.c_str());

  writer->String("logo");
  writer->String(banner.logo.c_str());

  writer->String("amounts");
  writer->StartArray();
  for (auto & amount : banner.amounts) {
    writer->Int(amount);
  }
  writer->EndArray();

  writer->String("social");
  writer->StartObject();
  for (auto & i : banner.social) {
    writer->String(i.first.c_str());
    writer->String(i.second.c_str());
  }
  writer->EndObject();

  writer->String("verified");
  writer->Bool(banner.verified);

  writer->String("provider");
  writer->String(banner.provider.c_str());

  writer->EndObject();
}

void saveToJson(JsonWriter* writer,
    const ledger::ActivityInfoFilter& info) {
  writer->StartObject();

  writer->String("id");
  writer->String(info.id.c_str());

  writer->String("excluded");
  writer->Int(info.excluded);

  writer->String("percent");
  writer->Uint(info.percent);

  writer->String("min_duration");
  writer->Uint64(info.min_duration);

  writer->String("reconcile_stamp");
  writer->Uint64(info.reconcile_stamp);

  writer->String("order_by");
  writer->StartObject();
  for (auto & i : info.order_by) {
    writer->String(i.first.c_str());
    writer->Bool(i.second);
  }
  writer->EndObject();

  writer->String("non_verified");
  writer->Bool(info.non_verified);

  writer->String("min_visits");
  writer->Uint(info.min_visits);

  writer->EndObject();
}

void saveToJson(JsonWriter* writer, const ledger::WalletInfo& info) {
  writer->StartObject();

  writer->String("altcurrency_");
  writer->String(info.altcurrency_.c_str());

  writer->String("probi_");
  writer->String(info.probi_.c_str());

  writer->String("balance_");
  writer->Double(info.balance_);

  writer->String("fee_amount_");
  writer->Double(info.fee_amount_);

  writer->String("rates_");
  writer->StartObject();
  for (const auto& rate : info.rates_) {
    writer->String(rate.first.c_str());
    writer->Double(rate.second);
  }
  writer->EndObject();

  writer->String("parameters_choices_");
  writer->StartArray();
  for (const auto& choice : info.parameters_choices_) {
    writer->Double(choice);
  }
  writer->EndArray();

  writer->String("parameters_range_");
  writer->StartArray();
  for (const auto& range : info.parameters_range_) {
    writer->Double(range);
  }
  writer->EndArray();

  writer->String("parameters_days_");
  writer->Uint(info.parameters_days_);

  writer->String("grants_");
  writer->StartArray();
  for (const auto& grant : info.grants_) {
    saveToJson(writer, grant);
  }
  writer->EndArray();

  writer->EndObject();
}

void saveToJson(JsonWriter* writer,
    const ledger::AutoContributeProps& props) {
  writer->StartObject();

  writer->String("enabled_contribute");
  writer->Bool(props.enabled_contribute);

  writer->String("contribution_min_time");
  writer->Uint64(props.contribution_min_time);

  writer->String("contribution_min_visits");
  writer->Int(props.contribution_min_visits);

  writer->String("contribution_non_verified");
  writer->Bool(props.contribution_non_verified);

  writer->String("contribution_videos");
  writer->Bool(props.contribution_videos);

  writer->String("reconcile_stamp");
  writer->Uint64(props.reconcile_stamp);

  writer->EndObject();
}

void saveToJson(JsonWriter* writer, const ledger::ReconcileInfo& data) {
  writer->StartObject();

  writer->String("viewingId");
  writer->String(data.viewingId_.c_str());

  writer->String("amount");
  writer->String(data.amount_.c_str());

  writer->String("retry_step");
  writer->Int(data.retry_step_);

  writer->String("retry_level");
  writer->Int(data.retry_level_);

  writer->EndObject();
}

void saveToJson(JsonWriter* writer, const ledger::RewardsInternalsInfo& info) {
  writer->StartObject();

  writer->String("payment_id");
  writer->String(info.payment_id.c_str());

  writer->String("is_key_info_seed_valid");
  writer->Bool(info.is_key_info_seed_valid);

  writer->String("current_reconciles");
  writer->StartArray();
  for (const auto& reconcile : info.current_reconciles)
    saveToJson(writer, reconcile.second);
  writer->EndArray();

  writer->String("persona_id");
  writer->String(info.persona_id.c_str());

  writer->String("user_id");
  writer->String(info.user_id.c_str());

  writer->String("boot_stamp");
  writer->Uint64(info.boot_stamp);

  writer->EndObject();
}

}  // namespace braveledger_bat_helper

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <sstream>
#include <random>
#include <utility>
#include <iomanip>
#include <ctime>
#include <memory>
#include <iostream>

#if defined CHROMIUM_BUILD
//#include "base/values.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/files/file_util.h"
#include "base/sequenced_task_runner.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "browser_thread.h"
#endif

//tweetnacl
#include "tweetnacl.h"

//boringssl
#include <openssl/hkdf.h>
#include <openssl/digest.h>
#include <openssl/sha.h>
#include <openssl/base64.h>


//rapidjson
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"


#include "bat_helper.h"
#include "static_values.h"


// to do debug
//#include <stdio.h>
//#include <stdlib.h>
//#include "base/android/apk_assets.h"
//#include "v8/include/v8.h"
//#include "v8/include/libplatform/libplatform.h"
//


namespace braveledger_bat_helper {

  REQUEST_CREDENTIALS_ST::REQUEST_CREDENTIALS_ST() {}

  REQUEST_CREDENTIALS_ST::~REQUEST_CREDENTIALS_ST() {}

  /////////////////////////////////////////////////////////////////////////////
  RECONCILE_PAYLOAD_ST::RECONCILE_PAYLOAD_ST() {}

  RECONCILE_PAYLOAD_ST::~RECONCILE_PAYLOAD_ST() {}

  /////////////////////////////////////////////////////////////////////////////
  WALLET_INFO_ST::WALLET_INFO_ST() {}

  WALLET_INFO_ST::~WALLET_INFO_ST() {}

  bool WALLET_INFO_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !( d["paymentId"].IsString() &&
        d["addressBAT"].IsString() &&
        d["addressBTC"].IsString() &&
        d["addressCARD_ID"].IsString() &&
        d["addressETH"].IsString() &&
        d["addressLTC"].IsString() &&
        d["keyInfoSeed"].IsString() );
    }

    //convert keyInfoSeed and check error
    std::string sKeyInfoSeed = d["keyInfoSeed"].GetString();
    error = ! getFromBase64(sKeyInfoSeed, keyInfoSeed_);
    
    if (false == error)
    {
      paymentId_ = d["paymentId"].GetString();
      addressBAT_ = d["addressBAT"].GetString();
      addressBTC_ = d["addressBTC"].GetString();
      addressCARD_ID_ = d["addressCARD_ID"].GetString();
      addressETH_ = d["addressETH"].GetString();
      addressLTC_ = d["addressLTC"].GetString();      
    }
    return !error;
  }
  
  void WALLET_INFO_ST::saveToJson(JsonWriter & writer) const
  {    
    writer.StartObject();

    writer.String("paymentId");
    writer.String(paymentId_.c_str());

    writer.String("addressBAT");
    writer.String(addressBAT_.c_str());

    writer.String("addressBTC");
    writer.String(addressBTC_.c_str());

    writer.String("addressCARD_ID");
    writer.String(addressCARD_ID_.c_str());

    writer.String("addressETH");
    writer.String(addressETH_.c_str());

    writer.String("addressLTC");
    writer.String(addressLTC_.c_str());

    writer.String("keyInfoSeed");
    writer.String(getBase64 (keyInfoSeed_).c_str());

    writer.EndObject();    
  }

  /////////////////////////////////////////////////////////////////////////////
  UNSIGNED_TX::UNSIGNED_TX() {}

  UNSIGNED_TX::~UNSIGNED_TX() {}

  /////////////////////////////////////////////////////////////////////////////
  TRANSACTION_BALLOT_ST::TRANSACTION_BALLOT_ST() :
    offset_(0) {}

  TRANSACTION_BALLOT_ST::~TRANSACTION_BALLOT_ST() {}

  bool TRANSACTION_BALLOT_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());    

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !( d["publisher"].IsString() &&
        d["offset"].IsUint() );
    }    
    
    if (false == error)
    {
      publisher_ = d["publisher"].GetString();
      offset_ = d["offset"].GetUint();
    }

    return !error;
  }  
  
  void TRANSACTION_BALLOT_ST::saveToJson(JsonWriter & writer) const
  {    
    writer.StartObject();

    writer.String("publisher");
    writer.String(publisher_.c_str());

    writer.String("offset");
    writer.Uint(offset_);

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

  bool TRANSACTION_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["viewingId"].IsString() &&
        d["surveyorId"].IsString() &&
        d["contribution_fiat_amount"].IsString() &&
        d["contribution_fiat_currency"].IsString() &&
        d["rates"].IsObject() &&
        d["rates"].HasMember("ETH") &&
        d["rates"].HasMember("LTC") &&
        d["rates"].HasMember("BTC") &&
        d["rates"].HasMember("USD") &&
        d["rates"].HasMember("EUR") &&
        d["contribution_altcurrency"].IsString() &&
        d["contribution_probi"].IsString() &&
        d["contribution_fee"].IsString() &&
        d["submissionStamp"].IsString() &&
        d["submissionId"].IsString() &&
        d["anonizeViewingId"].IsString() &&
        d["registrarVK"].IsString() &&
        d["masterUserToken"].IsString() &&
        d["surveyorIds"].IsArray() &&
        d["satoshis"].IsString() &&
        d["altCurrency"].IsString() &&
        d["probi"].IsString() &&
        d["votes"].IsUint() &&
        d["ballots"].IsArray());        
    }    
    
    if (false == error)
    {
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
      
      for ( auto & i : d["rates"].GetObject())
      {
        contribution_rates_.insert(std::make_pair(i.name.GetString(), i.value.GetDouble() ));
      }        

      for (auto & i : d["surveyorIds"].GetArray())
      {
        surveyorIds_.push_back(i.GetString());
      }      
        
      for (const auto & i : d["ballots"].GetArray() )
      {
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

  
  void TRANSACTION_ST::saveToJson(JsonWriter & writer) const
  {
    writer.StartObject();

    writer.String("viewingId");
    writer.String(viewingId_.c_str());

    writer.String("surveyorId");
    writer.String(surveyorId_.c_str());

    writer.String("contribution_fiat_amount");
    writer.String(contribution_fiat_amount_.c_str());

    writer.String("contribution_fiat_currency");
    writer.String(contribution_fiat_currency_.c_str());

    writer.String("rates");
    writer.StartObject();
    for (auto & p : contribution_rates_)
    {
      writer.String(p.first.c_str());
      writer.Double(p.second);
    }
    writer.EndObject();

    writer.String("contribution_altcurrency");
    writer.String(contribution_altcurrency_.c_str());

    writer.String("contribution_probi");
    writer.String(contribution_probi_.c_str());

    writer.String("contribution_fee");
    writer.String(contribution_fee_.c_str());

    writer.String("submissionStamp");
    writer.String(submissionStamp_.c_str());

    writer.String("submissionId");
    writer.String(submissionId_.c_str());

    writer.String("anonizeViewingId");
    writer.String(anonizeViewingId_.c_str());

    writer.String("registrarVK");
    writer.String(registrarVK_.c_str());

    writer.String("masterUserToken");
    writer.String(masterUserToken_.c_str());

    writer.String("surveyorIds");
    writer.StartArray();
    for (auto & i : surveyorIds_)
    {
      writer.String(i.c_str());
    }
    writer.EndArray();

    writer.String("satoshis");
    writer.String(satoshis_.c_str());

    writer.String("altCurrency");
    writer.String(altCurrency_.c_str());

    writer.String("probi");
    writer.String(probi_.c_str());

    writer.String("votes");
    writer.Uint(votes_);

    /* TODO: clarify if it needs to be serialized
    writer.String("ballots");
    writer.StartArray();
    for (auto & i : ballots_)
    {
      i.saveToJson(writer);
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

  bool BALLOT_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["viewingId"].IsString() &&
        d["surveyorId"].IsString() &&
        d["publisher"].IsString() &&
        d["offset"].IsUint() &&
        d["prepareBallot"].IsString() &&
        d["delayStamp"].IsUint64() );
    }
    
    if (false == error)
    {
      viewingId_ = d["viewingId"].GetString();
      surveyorId_ = d["surveyorId"].GetString();
      publisher_ = d["publisher"].GetString();
      offset_ = d["offset"].GetUint();
      prepareBallot_ = d["prepareBallot"].GetString();
      delayStamp_ = d["delayStamp"].GetUint64();
    }

    return !error;
  }  
  
  void BALLOT_ST::saveToJson(JsonWriter & writer) const
  {
    writer.StartObject();

    writer.String("viewingId");
    writer.String(viewingId_.c_str());

    writer.String("surveyorId");
    writer.String(surveyorId_.c_str());

    writer.String("publisher");
    writer.String(publisher_.c_str());

    writer.String("offset");
    writer.Uint(offset_);

    writer.String("prepareBallot");
    writer.String(prepareBallot_.c_str());

    writer.String("delayStamp");
    writer.Uint64(delayStamp_);

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  CLIENT_STATE_ST::CLIENT_STATE_ST():
    bootStamp_(0),
    reconcileStamp_(0),
    settings_(AD_FREE_SETTINGS),
    fee_amount_(0),
    days_(0)  {}

  CLIENT_STATE_ST::~CLIENT_STATE_ST() {}

  bool CLIENT_STATE_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser error or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["walletInfo"].IsObject() &&
        d["bootStamp"].IsUint64() &&
        d["reconcileStamp"].IsUint64() &&
        d["personaId"].IsString() &&
        d["userId"].IsString() &&
        d["registrarVK"].IsString() &&
        d["masterUserToken"].IsString() &&
        d["preFlight"].IsString() &&
        d["fee_currency"].IsString() &&
        d["settings"].IsString() &&
        d["fee_amount"].IsDouble() &&
        d["days"].IsUint() &&
        d["transactions"].IsArray() &&
        d["ballots"].IsArray() );
    }
    
    if (false == error)
    {
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
      days_ = d["days"].GetUint();

      for (const auto & i : d["transactions"].GetArray())
      {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        TRANSACTION_ST ta;
        ta.loadFromJson(sb.GetString());
        transactions_.push_back(ta);
      }

      for (const auto & i : d["ballots"].GetArray())
      {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        i.Accept(writer);

        BALLOT_ST b;
        b.loadFromJson(sb.GetString());
        ballots_.push_back(b);
      }    
    }

    return !error;
  }
  
  void CLIENT_STATE_ST::saveToJson(JsonWriter & writer) const
  {
    writer.StartObject();

    writer.String("walletInfo");
    walletInfo_.saveToJson(writer);

    writer.String("bootStamp");
    writer.Uint64(bootStamp_);

    writer.String("reconcileStamp");
    writer.Uint64(reconcileStamp_);

    writer.String("personaId");
    writer.String(personaId_.c_str());

    writer.String("userId");
    writer.String(userId_.c_str());

    writer.String("registrarVK");
    writer.String(registrarVK_.c_str());

    writer.String("masterUserToken");
    writer.String(masterUserToken_.c_str());

    writer.String("preFlight");
    writer.String(preFlight_.c_str());

    writer.String("fee_currency");
    writer.String(fee_currency_.c_str());

    writer.String("settings");
    writer.String(settings_.c_str());

    writer.String("fee_amount");
    writer.Double(fee_amount_);

    writer.String("days");
    writer.Uint(days_);

    writer.String("transactions");
    writer.StartArray();
    for (auto & t : transactions_)
    {
      t.saveToJson(writer);
    }
    writer.EndArray();

    writer.String("ballots");
    writer.StartArray();
    for (auto & b : ballots_)
    {
      b.saveToJson(writer);
    }
    writer.EndArray();

    writer.String("ruleset");
    writer.String(ruleset_.c_str());

    writer.String("rulesetV2");
    writer.String(rulesetV2_.c_str());    

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  PUBLISHER_STATE_ST::PUBLISHER_STATE_ST():
    min_pubslisher_duration_(braveledger_ledger::_default_min_pubslisher_duration),
    min_visits_(1),
    allow_non_verified_(true) {}

  PUBLISHER_STATE_ST::~PUBLISHER_STATE_ST() {}

  bool PUBLISHER_STATE_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());
    
    //has parser errors or wrong types
    bool error = d.HasParseError();       
    if (false == error)
    {
      error = !(d["min_pubslisher_duration"].IsUint() &&
        d["min_visits"].IsUint() &&
        d["allow_non_verified"].IsBool());
    }
    
    if (false == error)
    {
      min_pubslisher_duration_ = d["min_pubslisher_duration"].GetUint();
      min_visits_ = d["min_visits"].GetUint();
      allow_non_verified_ = d["allow_non_verified"].GetBool();
    }
    
    return !error;
  }  
  
  void PUBLISHER_STATE_ST::saveToJson(JsonWriter & writer) const
  {    
    writer.StartObject();
    
    writer.String("min_pubslisher_duration");
    writer.Uint(min_pubslisher_duration_);

    writer.String("min_visits");
    writer.Uint(min_visits_);

    writer.String("allow_non_verified");
    writer.Bool(allow_non_verified_);
    
    writer.EndObject();   
  }

  /////////////////////////////////////////////////////////////////////////////
  PUBLISHER_ST::PUBLISHER_ST():
    duration_(0),
    score_(0),
    visits_(0),
    verified_(false),
    exclude_(false),
    pinPercentage_(false),
    verifiedTimeStamp_(0),
    percent_(0),
    deleted_(false),
    weight_(0) {}

  PUBLISHER_ST::PUBLISHER_ST(const PUBLISHER_ST& publisher) :
    duration_(publisher.duration_),
    favicon_url_(publisher.favicon_url_),
    score_(publisher.score_),
    visits_(publisher.visits_),
    verified_(publisher.verified_),
    exclude_(publisher.exclude_),
    pinPercentage_(publisher.pinPercentage_),
    verifiedTimeStamp_(publisher.verifiedTimeStamp_),
    percent_(publisher.percent_),
    deleted_(publisher.deleted_),
    weight_(publisher.weight_)
    {}

  PUBLISHER_ST::~PUBLISHER_ST() {}

  bool PUBLISHER_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["duration"].IsUint64() &&
        d["favicon_url"].IsString() &&
        d["score"].IsDouble() &&
        d["visits"].IsUint() &&
        d["verified"].IsBool() &&
        d["exclude"].IsBool() &&
        d["pinPercentage"].IsBool() &&
        d["verifiedTimeStamp"].IsUint64() &&
        d["percent"].IsUint() &&
        d["deleted"].IsBool() &&
        d["weight"].IsDouble() );
    }

    if (false == error)
    {
      //TODO: Uint64 or string?
      duration_ = d["duration"].GetUint64();
      favicon_url_ = d["favicon_url"].GetString();
      score_ = d["score"].GetDouble();
      visits_ = d["visits"].GetUint();
      verified_ = d["verified"].GetBool();
      exclude_ = d["exclude"].GetBool();
      pinPercentage_ = d["pinPercentage"].GetBool();
      verifiedTimeStamp_ = d["verifiedTimeStamp"].GetUint64();
      percent_ = d["percent"].GetUint();
      deleted_ = d["deleted"].GetBool();
      weight_ = d["weight"].GetDouble();
    }
    
    return !error;
  }
  
  void PUBLISHER_ST::saveToJson(JsonWriter & writer) const
  {
    writer.StartObject();

    //TODO: uint64 or string
    writer.String("duration");
    writer.Uint64(duration_);

    writer.String("favicon_url");
    writer.String(favicon_url_.c_str());

    writer.String("score");
    writer.Double(score_);

    writer.String("visits");
    writer.Uint(visits_);

    writer.String("verified");
    writer.Bool(verified_);

    writer.String("exclude");
    writer.Bool(exclude_);

    writer.String("pinPercentage");
    writer.Bool(pinPercentage_);

    //TODO: uint64 or string
    writer.String("verifiedTimeStamp");
    writer.Uint64(verifiedTimeStamp_);

    writer.String("percent");
    writer.Uint(percent_);

    writer.String("deleted");
    writer.Bool(deleted_);

    writer.String("weight");
    writer.Double(weight_);

    writer.EndObject();
  }

  /////////////////////////////////////////////////////////////////////////////
  PUBLISHER_DATA_ST::PUBLISHER_DATA_ST() :
    daysSpent_(0),
    hoursSpent_(0),
    minutesSpent_(0),
    secondsSpent_(0) {}

  PUBLISHER_DATA_ST::PUBLISHER_DATA_ST(const PUBLISHER_DATA_ST& publisherData) :
    publisherKey_(publisherData.publisherKey_),
    publisher_(publisherData.publisher_),
    daysSpent_(publisherData.daysSpent_),
    hoursSpent_(publisherData.hoursSpent_),
    minutesSpent_(publisherData.minutesSpent_),
    secondsSpent_(publisherData.secondsSpent_) {}

  PUBLISHER_DATA_ST::~PUBLISHER_DATA_ST() {}

  bool PUBLISHER_DATA_ST::operator<(const PUBLISHER_DATA_ST &rhs) const {
    return publisher_.score_ > rhs.publisher_.score_;
  }

  /////////////////////////////////////////////////////////////////////////////
  WINNERS_ST::WINNERS_ST() :
    votes_(0) {}

  WINNERS_ST::~WINNERS_ST() {}

  /////////////////////////////////////////////////////////////////////////////
  WALLET_PROPERTIES_ST::WALLET_PROPERTIES_ST() {}

  WALLET_PROPERTIES_ST::~WALLET_PROPERTIES_ST() {}

  bool WALLET_PROPERTIES_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["altcurrency"].IsString() &&
        d["balance"].IsDouble() &&
        d["rates"].IsObject() &&
        d["parameters"].IsObject() );
    }

    if (false == error)
    {
      altcurrency_ = d["altcurrency"].GetString();
      balance_ = d["balance"].GetDouble();
      
      for (auto & i : d["rates"].GetObject())
      {
        rates_.insert(std::make_pair(i.name.GetString(), i.value.GetDouble()));
      }

      parameters_currency_ = d["parameters"]["adFree"]["currency"].GetString();
      parameters_fee_ = d["parameters"]["adFree"]["fee"]["BAT"].GetDouble();

      for (auto & i : d["parameters"]["adFree"]["choices"]["BAT"].GetArray())
      {
        parameters_choices_.push_back(i.GetDouble());
      }

      for (auto & i : d["parameters"]["adFree"]["range"]["BAT"].GetArray())
      {
        parameters_range_.push_back(i.GetDouble());
      }

      parameters_days_ = d["parameters"]["adFree"]["days"].GetUint();
    }
    return !error;
  }  

  /////////////////////////////////////////////////////////////////////////////
  FETCH_CALLBACK_EXTRA_DATA_ST::FETCH_CALLBACK_EXTRA_DATA_ST():
    value1(0),
    boolean1(true) {}

  FETCH_CALLBACK_EXTRA_DATA_ST::FETCH_CALLBACK_EXTRA_DATA_ST(const FETCH_CALLBACK_EXTRA_DATA_ST& extraData):
    value1(extraData.value1),
    string1(extraData.string1),
    string2(extraData.string2),
    string3(extraData.string3),
    string4(extraData.string4),
    string5(extraData.string5),
    boolean1(extraData.boolean1) {}

  FETCH_CALLBACK_EXTRA_DATA_ST::~FETCH_CALLBACK_EXTRA_DATA_ST() {}

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

  bool SURVEYOR_ST::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["signature"].IsString() &&
        d["surveyorId"].IsString() &&
        d["surveyVK"].IsString() &&
        d["registrarVK"].IsString() &&
        d["surveySK"].IsString() );
    }

    if (false == error)
    {
      signature_ = d["signature"].GetString();
      surveyorId_ = d["surveyorId"].GetString();
      surveyVK_ = d["surveyVK"].GetString();
      registrarVK_ = d["registrarVK"].GetString();
      surveySK_ = d["surveySK"].GetString();
    }

    return !error;
  }
  
  void SURVEYOR_ST::saveToJson(JsonWriter & writer) const
  {
    writer.StartObject();

    writer.String("signature");
    writer.String(signature_.c_str());

    writer.String("surveyorId");
    writer.String(surveyorId_.c_str());

    writer.String("surveyVK");
    writer.String(surveyVK_.c_str());

    writer.String("registrarVK");
    writer.String(registrarVK_.c_str());

    writer.String("surveySK");
    writer.String(surveySK_.c_str());

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
    publisher_(mediaPublisherInfo.publisher_),
    twitchEventInfo_(mediaPublisherInfo.twitchEventInfo_) {}

  MEDIA_PUBLISHER_INFO::~MEDIA_PUBLISHER_INFO() {}


  bool MEDIA_PUBLISHER_INFO::loadFromJson(const std::string & json)
  {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["publisherName"].IsString() &&
        d["publisherURL"].IsString() &&
        d["favIconURL"].IsString() &&
        d["channelName"].IsString() &&
        d["publisher"].IsString() &&
        d["twitch_event"].IsString() &&
        d["twitch_time"].IsString() &&
        d["twitch_status"].IsString());
    }

    if (false == error)
    {
      publisherName_ = d["publisherName"].GetString();
      publisherURL_ = d["publisherURL"].GetString();
      favIconURL_ = d["favIconURL"].GetString();
      channelName_ = d["channelName"].GetString();
      publisher_ = d["publisher"].GetString();
      twitchEventInfo_.event_ = d["twitch_event"].GetString();
      twitchEventInfo_.time_ = d["twitch_time"].GetString();
      twitchEventInfo_.status_ = d["twitch_status"].GetString();
    }
    return !error;
  }

  void MEDIA_PUBLISHER_INFO::saveToJson(JsonWriter & writer) const
  {
    writer.StartObject();

    writer.String("publisherName");
    writer.String(publisherName_.c_str());

    writer.String("publisherURL");
    writer.String(publisherURL_.c_str());

    writer.String("favIconURL");
    writer.String(favIconURL_.c_str());

    writer.String("channelName");
    writer.String(channelName_.c_str());

    writer.String("publisher");
    writer.String(publisher_.c_str());

    writer.String("twitch_event");
    writer.String(twitchEventInfo_.event_.c_str());

    writer.String("twitch_time");
    writer.String(twitchEventInfo_.time_.c_str());

    writer.String("twitch_status");
    writer.String(twitchEventInfo_.status_.c_str());

    writer.EndObject();
  }

/////////////////////////////////////////////////////////////////////////////
  BATCH_PROOF::BATCH_PROOF() {}

  BATCH_PROOF::~BATCH_PROOF() {}

/////////////////////////////////////////////////////////////////////////////
  void split(std::vector<std::string>& tmp, std::string query, char delimiter)
  {
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
    if (false == error)
    {      
      value = d[fieldName.c_str()].GetString();
    }
    return !error;  
}

  bool getJSONList(const std::string& fieldName, const std::string& json, std::vector<std::string> & value) {
    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError() || (false == ( d.HasMember(fieldName.c_str())  &&  d[fieldName.c_str()].IsArray() ) );
    if (false == error)
    {
      for (auto & i : d[fieldName.c_str()].GetArray())
      {
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
    if (false == error)
    {
      for (auto & i : d.GetArray())
      {
        const char * event_field = "event";
        std::map<std::string, std::string> eventmap;
      
        auto obj = i.GetObject();      
        if (obj.HasMember(event_field))
        {
          eventmap[event_field] = obj[event_field].GetString();
        }

        const char * props_field = "properties";      
        if (obj.HasMember(props_field))
        {
          eventmap[props_field] = "";

          const char * channel_field = "channel";
          if (obj[props_field].HasMember(channel_field)) {
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
    if (false == error)
    {
      for (auto & i : d.GetArray())
      {
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
    if (false == error)
    {
      error = !(d["rates"].IsObject() &&
        d["rates"].HasMember("ETH") &&
        d["rates"].HasMember("LTC") &&
        d["rates"].HasMember("BTC") &&
        d["rates"].HasMember("USD") &&
        d["rates"].HasMember("EUR"));
    }

    if (false == error)
    {
      for (auto & i : d["rates"].GetObject())
      {
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
    if (false == error)
    {
      error = !(d["paymentStamp"].IsDouble() &&
        d["probi"].IsString() &&
        d["altcurrency"].IsString() );
    }

    if (false == error)
    {
      double stamp = d["paymentStamp"].GetDouble();
      transaction.submissionStamp_ = std::to_string((unsigned long long)stamp);
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
    if (false == error)
    {
      error = !(d["unsignedTx"].IsObject());
    }

    if (false == error)
    {
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
    if (false == error)
    {
      error = !(d["properties"].IsObject());
    }

    if (false == error)
    {
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
    if (false == error)
    {
      error = !(d["wallet"].IsObject() &&
        d["payload"].IsObject() );
    }

    if (false == error)
    {
      walletInfo.paymentId_ = d["wallet"]["paymentId"].GetString();
      walletInfo.addressBAT_ = d["wallet"]["addresses"]["BAT"].GetString();
      walletInfo.addressCARD_ID_ = d["wallet"]["addresses"]["CARD_ID"].GetString();
      walletInfo.addressETH_ = d["wallet"]["addresses"]["ETH"].GetString();
      walletInfo.addressLTC_ = d["wallet"]["addresses"]["LTC"].GetString();

      days = d["payload"]["adFree"]["days"].GetUint();
      const auto & fee = d["payload"]["adFree"]["fee"].GetObject();
      auto itr = fee.MemberBegin();
      if (itr != fee.MemberEnd() )
      {
        fee_currency = itr->name.GetString();
        fee_amount = itr->value.GetDouble();
      }
    }
    return !error;
  }

  bool getJSONPublisherTimeStamp(const std::string& json, uint64_t& publisherTimestamp) {
    publisherTimestamp = 0;

    rapidjson::Document d;
    d.Parse(json.c_str());

    //has parser errors or wrong types
    bool error = d.HasParseError();
    if (false == error)
    {
      error = !(d["timestamp"].IsUint64());
    }

    if (false == error)
    {
      publisherTimestamp = d["timestamp"].GetUint64();
    }
    return !error;  
  }

  std::vector<uint8_t> generateSeed() {
    //std::ostringstream seedStr;

    std::vector<uint8_t> vSeed(SEED_LENGTH);
    std::random_device r;
    std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    auto rand = std::bind(std::uniform_int_distribution<>(0, UCHAR_MAX),
                            std::mt19937(seed));

    //std::generate_n(std::ostream_iterator<int>(seedStr, ","), seedLength, rand);
    std::generate_n(vSeed.begin(), SEED_LENGTH, rand);
    /*for (size_t i = 0; i < vSeed.size(); i++) {
      if (0 != i) {
        seedStr << ",";
      }
      seedStr << vSeed[i];
    }
    std::string res = seedStr.str();
    //if (!res.empty()) {
    //  res.erase(res.end() - 1);
    //}
    LOG(ERROR) << res;

    return res;*/
    return vSeed;
  }

  std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed) {
    DCHECK(!seed.empty());
    std::vector<uint8_t> out(SEED_LENGTH);
    //uint8_t out[SEED_LENGTH];
    //to do debug
    /*std::ostringstream seedStr1;
    for (size_t i = 0; i < SEED_LENGTH; i++) {
      if (0 != i) {
        seedStr1 << ",";
      }
      seedStr1 << (int)seed[i];
    }
    LOG(ERROR) << "!!!seed == " << seedStr1.str();*/
    //
    int hkdfRes = HKDF(&out.front(), SEED_LENGTH, EVP_sha512(), &seed.front(), seed.size(),
      braveledger_ledger::g_hkdfSalt, SALT_LENGTH, nullptr, 0);

    DCHECK(hkdfRes);
    DCHECK(!seed.empty());

    //to do debug
    /*std::ostringstream seedStr;
    for (size_t i = 0; i < SEED_LENGTH; i++) {
      if (0 != i) {
        seedStr << ",";
      }
      seedStr << (int)out[i];
    }
    LOG(ERROR) << "!!!hkdfRes == " << hkdfRes << ", out == " << seedStr.str();*/
    //

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
    //to do debug
    /*std::ostringstream publicStr;
    for (size_t i = 0; i < crypto_sign_PUBLICKEYBYTES; i++) {
      if (0 != i) {
        publicStr << ",";
      }
      publicStr << (int)outPublic[i];
    }
    std::ostringstream secretStr;
    for (size_t i = 0; i < crypto_sign_SECRETKEYBYTES; i++) {
      if (0 != i) {
        secretStr << ",";
      }
      secretStr << (int)outSecret[i];
    }
    LOG(ERROR) << "!!!publicStr == " << publicStr.str();
    LOG(ERROR) << "!!!secretStr == " << secretStr.str();*/
  }

  std::string uint8ToHex(const std::vector<uint8_t>& in) {
    std::ostringstream res;
    for (size_t i = 0; i < in.size(); i++) {
      res << std::setfill('0') << std::setw(sizeof(uint8_t) * 2)
         << std::hex << (int)in[i];
    }
    return res.str();
  }


  std::string stringify(std::string* keys, std::string* values, const unsigned int& size) {
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
  
    for (unsigned int i = 0; i < size; i++)
    {
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

    writer.String("amout");
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

    if (succeded)
    {
      out.resize(size);
      int numDecBytes = EVP_DecodeBase64(&out.front(), &size, size, (const uint8_t*)in.c_str(), in.length());
      DCHECK(numDecBytes != 0);
      LOG(ERROR) << "!!!decoded size == " << size;

      if (0 == numDecBytes)
      {
        succeded = false;
        out.clear();
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
    uint64_t signedMsgSize = 0;
    crypto_sign(&signedMsg.front(), &signedMsgSize, (const unsigned char*)message.c_str(),
      (uint64_t)message.length(), &secretKey.front());
    std::vector<uint8_t> signature(crypto_sign_BYTES);
    std::copy(signedMsg.begin(), signedMsg.begin() + crypto_sign_BYTES, signature.begin());

    return "keyId=\"" + keyId + "\",algorithm=\"" + SIGNATURE_ALGORITHM +
      "\",headers=\"" + headers + "\",signature=\"" + getBase64(signature) + "\"";
  }

  uint64_t currentTime() {
    return time(0);
  }

  void writeStateFile(const std::string& data) {
  #if defined CHROMIUM_BUILD
    base::FilePath dirToSave;
    base::PathService::Get(base::DIR_HOME, &dirToSave);
    dirToSave = dirToSave.Append(LEDGER_STATE_FILENAME);

    int succeded = base::WriteFile(dirToSave, data.c_str(), data.length());
    LOG(ERROR)<<"writeStateFile to: " << dirToSave << " : " << data.length() << " : " <<succeded;
    assert(succeded != -1);
    #endif
  }

  void readStateFile(ReadStateCallback callback) {
  #if defined CHROMIUM_BUILD
    base::FilePath dirToSave;
    base::PathService::Get(base::DIR_HOME, &dirToSave);
    dirToSave = dirToSave.Append(LEDGER_STATE_FILENAME);
    int64_t file_size = 0;
    if (!GetFileSize(dirToSave, &file_size)) {

      callback.Run(false, CLIENT_STATE_ST());

      return;
    }
    std::vector<char> data(file_size + 1);
    if (-1 != base::ReadFile(dirToSave, &data.front(), file_size)) {
      data[file_size] = '\0';
      CLIENT_STATE_ST state;
      braveledger_bat_helper::loadFromJson(state, &data.front());
      callback.Run(true, state);
      return;
    }

    callback.Run(false, CLIENT_STATE_ST());
  #endif
  }

  void writePublisherStateFile(const std::string& data) {
  #if defined CHROMIUM_BUILD
    base::FilePath dirToSave;
    base::PathService::Get(base::DIR_HOME, &dirToSave);
    dirToSave = dirToSave.Append(LEDGER_PUBLISHER_STATE_FILENAME);

    int succeded = base::WriteFile(dirToSave, data.c_str(), data.length());  
    LOG(ERROR)<<"writeStateFile to: " << dirToSave << " : " << data.length() << " : " <<succeded;
    assert(succeded != -1);
  #else

  #endif
  }

  void readPublisherStateFile(ReadPublisherStateCallback callback) {
  #if defined CHROMIUM_BUILD
    base::FilePath dirToSave;
    base::PathService::Get(base::DIR_HOME, &dirToSave);
    dirToSave = dirToSave.Append(LEDGER_PUBLISHER_STATE_FILENAME);
    int64_t file_size = 0;
    if (!GetFileSize(dirToSave, &file_size)) {

      callback.Run(false, PUBLISHER_STATE_ST());

      return;
    }
    std::vector<char> data(file_size + 1);
    if (-1 != base::ReadFile(dirToSave, &data.front(), file_size)) {
      data[file_size] = '\0';
      PUBLISHER_STATE_ST state;    
      braveledger_bat_helper::loadFromJson(state, &data.front());
      callback.Run(true, state);

      return;
    }

    callback.Run(false, PUBLISHER_STATE_ST());
  #else

  #endif
  }

  void saveState(const CLIENT_STATE_ST& state) {
  
    std::string data;
    braveledger_bat_helper::saveToJson(state, data);
    //LOG(ERROR) << "!!!saveState == " << data;
  #if defined CHROMIUM_BUILD
    scoped_refptr<base::SequencedTaskRunner> task_runner =
       base::CreateSequencedTaskRunnerWithTraits(
           {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    task_runner->PostTask(FROM_HERE, base::Bind(&writeStateFile, data));
  #else
  #endif
  }

  void loadState(ReadStateCallback callback) {
  #if defined CHROMIUM_BUILD
    scoped_refptr<base::SequencedTaskRunner> task_runner =
       base::CreateSequencedTaskRunnerWithTraits(
           {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    task_runner->PostTask(FROM_HERE, base::Bind(&readStateFile, callback));
  #else
  #endif
  }

  void savePublisherState(const PUBLISHER_STATE_ST& state) {
    std::string data;
    braveledger_bat_helper::saveToJson(state, data);
  #if defined CHROMIUM_BUILD
    scoped_refptr<base::SequencedTaskRunner> task_runner =
       base::CreateSequencedTaskRunnerWithTraits(
           {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    task_runner->PostTask(FROM_HERE, base::Bind(&writePublisherStateFile, data));
  #else
  #endif
  }

  void loadPublisherState(ReadPublisherStateCallback callback) {
  #if defined CHROMIUM_BUILD
    scoped_refptr<base::SequencedTaskRunner> task_runner =
       base::CreateSequencedTaskRunnerWithTraits(
           {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    task_runner->PostTask(FROM_HERE, base::Bind(&readPublisherStateFile, callback));
  #else
  #endif
  }

  void getUrlQueryParts(const std::string& query, std::map<std::string, std::string>& parts) {
    std::vector<std::string> vars;
    split(vars, query, '&');
    for (size_t i = 0; i < vars.size(); i++) {
      std::vector<std::string> var;
      split(var, vars[i], '=');
      if (var.size() != 2) {
        continue;
      }
      std::string varName;
      std::string varValue;
      DecodeURLChars(var[0], varName);
      DecodeURLChars(var[1], varValue);
      parts[varName] = varValue;
    }
  }

  void getTwitchParts(const std::string& query, std::vector<std::map<std::string, std::string>>& parts) {
    size_t pos = query.find("data=");
    if (std::string::npos != pos && query.length() > 5) {
      std::string varValue;
      DecodeURLChars(query.substr(5), varValue);
      std::vector<uint8_t> decoded;
      bool succeded = braveledger_bat_helper::getFromBase64(varValue, decoded);
      if (succeded)
      {
        decoded.push_back((uint8_t)'\0');
        braveledger_bat_helper::getJSONTwitchProperties((char*)&decoded.front(), parts);
      }
      else
      {
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
              std::remove(idAddition.begin(), idAddition.end(), 'v');
              id += "_vod_" + idAddition;
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

  uint64_t getMediaDuration(const std::map<std::string, std::string>& data, const std::string& mediaKey, const std::string& type) {
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
        duration = (uint64_t)(tempTime * 1000.0);
      }
    } else if (TWITCH_MEDIA_TYPE == type) {
      // We set the correct duration for twitch in BatGetMedia class
      duration = 0;
    }

    return duration;
  }


  void DecodeURLChars(const std::string& input, std::string& output) {
  #if defined CHROMIUM_BUILD
    url::RawCanonOutputW<1024> canonOutput;
    url::DecodeURLEscapeSequences(input.c_str(), input.length(), &canonOutput);
    output = base::UTF16ToUTF8(base::StringPiece16(canonOutput.data(), canonOutput.length()));
  #else
    //TODO: to implement  
  #endif
  }


  std::string GenerateGUID()
  {
  #if defined CHROMIUM_BUILD
    return base::GenerateGUID();
  #else
    //TODO: to implement
    return "please implement";
  #endif
  }

  void encodeURIComponent(const std::string & instr, std::string & outstr)
  {
  #if defined CHROMIUM_BUILD
    url::StdStringCanonOutput surveyorIdCanon(&outstr);
    url::EncodeURIComponent(instr.c_str(), instr.length(), &surveyorIdCanon);
    surveyorIdCanon.Complete();
  #else
    //TODO: to implement
    assert(false);
  #endif
  }

  void getDbFile(const std::string & id, std::string & pubDbPath)
  {
  #if defined CHROMIUM_BUILD
    base::FilePath dbFilePath;
    base::PathService::Get(base::DIR_HOME, &dbFilePath);
    dbFilePath = dbFilePath.Append(id);
    pubDbPath = dbFilePath.value();
  #else
    //TODO: to implement
    assert(false);
  #endif
  }

  // Enable emscripten calls
  /*void readEmscriptenInternal() {
    base::MemoryMappedFile::Region region_out;
    int fd_out = base::android::OpenApkAsset("assets/anonize2-jumbo.mp3", &region_out);
    if (fd_out < 0) {
      LOG(ERROR) << "readEmscripten error: Cannot open assets/anonize2-jumbo.dat";
      return;
    }

    base::File file(fd_out);
    base::MemoryMappedFile* adblock_mmap_ = new base::MemoryMappedFile();
    if (!adblock_mmap_->Initialize(std::move(file), region_out)) {
      LOG(ERROR) << "InitAdBlock: Cannot init memory mapped file";
      return;
    }
    std::vector<char> data(adblock_mmap_->length() + 1);
    ::memcpy(&data.front(), adblock_mmap_->data(), adblock_mmap_->length());
    data[adblock_mmap_->length()] = '\0';
    //LOG(ERROR) << "!!!file == " << &data.front();
    LOG(ERROR) << "!!!length == " << data.size();
    delete adblock_mmap_;

    std::string toExecute = (std::string)&data.front() + "\r\nvar init = cwrap('initAnonize', '', '')\r\ninit()";
    toExecute += "\r\nvar makeCred = cwrap('makeCred', 'string', [ 'string' ])\r\nmakeCred('6d1219ab4ac45a5928323eb196ed62a')";

    // V8 init
    v8::V8::InitializeICU();
  //  v8::Platform *platform = v8::platform::CreateDefaultPlatform();
  //  v8::V8::InitializePlatform(platform);

    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());

    v8::V8::Initialize();

    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    // Create a new Isolate and make it the current one.
    v8::Isolate* isolate = v8::Isolate::New(create_params);

    //v8::Persistent<v8::String> test;

    {
      v8::Isolate::Scope isolate_scope(isolate);

      // Create a stack-allocated handle scope.
      v8::HandleScope handle_scope(isolate);

      // Create a new context.
      v8::Local<v8::Context> context = v8::Context::New(isolate);

      // Enter the context for compiling and running the hello world script.
      v8::Context::Scope context_scope(context);


      //test.Reset(isolate, v8::String::NewFromUtf8(isolate, "Hello' + ', World!'"));
      //test.SetWeak(&test, weak_callback, v8::WeakCallbackType::kParameter);


      // Create a string containing the JavaScript source code.
      v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, (const char*)toExecute.c_str());

      // Compile the source code.
      v8::TryCatch try_catch(isolate);

      // Compile the script and check for errors.
      v8::Local<v8::Script> compiled_script;
      if (!v8::Script::Compile(context, source).ToLocal(&compiled_script)) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        LOG(ERROR) << "!!!error == " << *error;
        // The script failed to compile; bail out.
      }

      // Run the script to get the result.
      v8::Local<v8::Value> result = compiled_script->Run();

      // Convert the result to an UTF8 string and print it.
      v8::String::Utf8Value utf8(isolate, result);
      LOG(ERROR) << "!!!result == " << *utf8;
    }

    isolate->LowMemoryNotification();

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
  }

  void readEmscripten() {
    scoped_refptr<base::SequencedTaskRunner> task_runner =
       base::CreateSequencedTaskRunnerWithTraits(
           {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    task_runner->PostTask(FROM_HERE, base::Bind(&braveledger_bat_helper::readEmscriptenInternal));
  }*/
  //

} //namespace braveledger_bat_helper 


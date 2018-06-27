/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_HELPER_H_
#define BRAVELEDGER_BAT_HELPER_H_

#include <string>
#include <vector>
#include <map>
#include <cassert>


#if defined CHROMIUM_BUILD
#include "base/callback.h"
#include "base/bind.h"
#else
#include <functional>
#include <iostream>

#define DCHECK assert
#define LOG(LEVEL) std::cerr<< std::endl<< #LEVEL << ": "
#endif

#include "static_values.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace braveledger_bat_helper {

  using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

  template <typename T> void  saveToJson(const T& t, std::string & json)
  {
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    t.saveToJson(writer);
    json = buffer.GetString();
  }

  //return: parsing status:  true = succeded, false = failed 
  template <typename T> bool  loadFromJson( T& t, const std::string & json)
  {    
    bool succeded = t.loadFromJson(json);
    if (!succeded) {
      LOG(ERROR) << "Failed to parse:" << json << std::endl;
    }
    return succeded;
  }

  struct REQUEST_CREDENTIALS_ST {  
    REQUEST_CREDENTIALS_ST();
    ~REQUEST_CREDENTIALS_ST();

    std::string proof_;
    std::string requestType_;
    std::string request_body_currency_;
    std::string request_body_label_;
    std::string request_body_publicKey_;
    std::string request_body_octets_;
    std::string request_headers_digest_;
    std::string request_headers_signature_;
  };

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
    ~WALLET_INFO_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string    
    void saveToJson(JsonWriter & writer) const;
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

    //load from json string
    bool loadFromJson(const std::string & json);    

    //save to json string
    void saveToJson(JsonWriter & writer) const;
	
    std::string publisher_;
    unsigned int offset_ = 0u;
  };

  struct TRANSACTION_ST {
    TRANSACTION_ST();
    TRANSACTION_ST(const TRANSACTION_ST& transaction);
    ~TRANSACTION_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;

    std::string viewingId_;
    std::string surveyorId_;
    std::string contribution_fiat_amount_;
    std::string contribution_fiat_currency_;
    std::map<std::string, double> contribution_rates_;
    std::string contribution_altcurrency_;
    std::string contribution_probi_;
    std::string contribution_fee_;
    std::string submissionStamp_;
    std::string submissionId_;
    std::string anonizeViewingId_;
    std::string registrarVK_;
    std::string masterUserToken_;
    std::vector<std::string> surveyorIds_;
    std::string satoshis_;
    std::string altCurrency_;
    std::string probi_;
    unsigned int votes_ = 0u;
    std::vector<TRANSACTION_BALLOT_ST> ballots_;
  };

  struct BALLOT_ST {
    BALLOT_ST();
    BALLOT_ST(const BALLOT_ST& ballot);
    ~BALLOT_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;
    std::string viewingId_;
    std::string surveyorId_;
    std::string publisher_;
    unsigned int offset_ = 0u;
    std::string prepareBallot_;
    std::string proofBallot_;
    uint64_t delayStamp_ = 0u;
  };

  struct CLIENT_STATE_ST {
    CLIENT_STATE_ST();
    ~CLIENT_STATE_ST();
    
    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;

    WALLET_INFO_ST walletInfo_;
    uint64_t bootStamp_ = 0u;
    uint64_t reconcileStamp_ = 0u;
    std::string personaId_;
    std::string userId_;
    std::string registrarVK_;
    std::string masterUserToken_;
    std::string preFlight_;
    std::string fee_currency_;
    std::string settings_= AD_FREE_SETTINGS;
    double fee_amount_ = .0;
    unsigned int days_ = 0u;
    std::vector<TRANSACTION_ST> transactions_;
    std::vector<BALLOT_ST> ballots_;
    std::string ruleset_;
    std::string rulesetV2_;
  };

  struct PUBLISHER_STATE_ST {
    PUBLISHER_STATE_ST();
    ~PUBLISHER_STATE_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;
    unsigned int min_pubslisher_duration_ = braveledger_ledger::_default_min_pubslisher_duration;  // In milliseconds
    unsigned int min_visits_ = 1u;
    bool allow_non_verified_ = true;
  };

  struct PUBLISHER_ST {
    PUBLISHER_ST();
    PUBLISHER_ST(const PUBLISHER_ST& publisher);
    ~PUBLISHER_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;

    uint64_t duration_ = 0u;
    std::string favicon_url_;
    double score_ = .0;
    unsigned int visits_ = 0;
    bool verified_ = false;
    bool exclude_ = false;
    bool pinPercentage_ = false;
    uint64_t verifiedTimeStamp_ = 0u;
    unsigned int percent_ = 0;
    bool deleted_ = false;
    double weight_ = .0;
  };

  struct PUBLISHER_DATA_ST {
    PUBLISHER_DATA_ST();
    PUBLISHER_DATA_ST(const PUBLISHER_DATA_ST& publisherData);
    ~PUBLISHER_DATA_ST();

    bool operator<(const PUBLISHER_DATA_ST &rhs) const;

    std::string publisherKey_;
    PUBLISHER_ST publisher_;
    unsigned int daysSpent_ = 0;
    unsigned int hoursSpent_ = 0;
    unsigned int minutesSpent_ = 0;
    unsigned int secondsSpent_ = 0;
  };

  struct WINNERS_ST {
    WINNERS_ST();
    ~WINNERS_ST();

    PUBLISHER_DATA_ST publisher_data_;
    unsigned int votes_ = 0;
  };

  struct WALLET_PROPERTIES_ST {
    WALLET_PROPERTIES_ST();
    ~WALLET_PROPERTIES_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    std::string altcurrency_;
    double balance_;
    std::map<std::string, double> rates_;
    std::string parameters_currency_;
    double parameters_fee_;
    std::vector<double> parameters_choices_;
    std::vector<double> parameters_range_;
    unsigned int parameters_days_;
  };

  struct FETCH_CALLBACK_EXTRA_DATA_ST {
    FETCH_CALLBACK_EXTRA_DATA_ST();
    FETCH_CALLBACK_EXTRA_DATA_ST(const FETCH_CALLBACK_EXTRA_DATA_ST&);
    ~FETCH_CALLBACK_EXTRA_DATA_ST();

    uint64_t value1 = 0u;
    std::string string1;
    std::string string2;
    std::string string3;
    std::string string4;
    std::string string5;
    bool boolean1 = true;
  };

  struct SURVEYOR_INFO_ST {
    SURVEYOR_INFO_ST();
    ~SURVEYOR_INFO_ST();

    std::string surveyorId_;
  };

  struct SURVEYOR_ST {
    SURVEYOR_ST();
    ~SURVEYOR_ST();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;

    std::string signature_;
    std::string surveyorId_;
    std::string surveyVK_;
    std::string registrarVK_;
    std::string surveySK_;
  };

  
  /*
    The struct is serialized/deserialized from/into JSON as part of MEDIA_PUBLISHER_INFO
  */
  struct TWITCH_EVENT_INFO {
    TWITCH_EVENT_INFO();
    TWITCH_EVENT_INFO(const TWITCH_EVENT_INFO&);
    ~TWITCH_EVENT_INFO();
    
    std::string event_;
    std::string time_;
    std::string status_;
  };

  struct MEDIA_PUBLISHER_INFO {
    MEDIA_PUBLISHER_INFO();
    MEDIA_PUBLISHER_INFO(const MEDIA_PUBLISHER_INFO&);
    ~MEDIA_PUBLISHER_INFO();

    //load from json string
    bool loadFromJson(const std::string & json);

    //save to json string
    void saveToJson(JsonWriter & writer) const;

    std::string publisherName_;
    std::string publisherURL_;
    std::string favIconURL_;
    std::string channelName_;
    std::string publisher_;
    TWITCH_EVENT_INFO twitchEventInfo_;
  };

  struct BATCH_PROOF {
    BATCH_PROOF();
    ~BATCH_PROOF();

    TRANSACTION_ST transaction_;
    BALLOT_ST ballot_;
  };

  enum URL_METHOD {
    GET = 0,
    PUT = 1,
    POST = 2
  };
  

  typedef void (FetchCallbackSignature) (bool, const std::string&, const FETCH_CALLBACK_EXTRA_DATA_ST&);
  typedef void (ReadStateCallbackSignature) (bool, const CLIENT_STATE_ST&);
  typedef void (ReadPublisherStateCallbackSignature) (bool, const PUBLISHER_STATE_ST&) ;
  typedef void (SimpleCallbackSignature) (const std::string&) ;

#if defined CHROMIUM_BUILD
  typedef base::Callback<FetchCallbackSignature> FetchCallback;
  typedef base::Callback<ReadStateCallbackSignature> ReadStateCallback;
  typedef base::Callback<ReadPublisherStateCallbackSignature> ReadPublisherStateCallback;
  typedef base::Callback<SimpleCallbackSignature> SimpleCallback;

  //Binds a member function of an instance of the type (Base) which has a signature (Signature) to a callback wrapper.  
  template <typename BaseType, typename Signature >
  static base::Callback<Signature> bat_mem_fun_binder(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    return base::Bind(ptr_to_mem, base::Unretained(&instance));
  }

  //working with Chromium Callback.Run()
  template <typename ReturnValue, typename Runnable, typename... Args>
  ReturnValue run_runnable(Runnable & runnable, Args... args)
  {
    return runnable.Run(args...);
  }

  #define bat_mem_fun_binder1 bat_mem_fun_binder
  #define bat_mem_fun_binder2 bat_mem_fun_binder
  #define bat_mem_fun_binder3 bat_mem_fun_binder

#else
  typedef std::function<FetchCallbackSignature> FetchCallback;
  typedef std::function<ReadStateCallbackSignature> ReadStateCallback;
  typedef std::function<ReadPublisherStateCallbackSignature> ReadPublisherStateCallback;
  typedef std::function<SimpleCallbackSignature> SimpleCallback;

  //Binds a member function of an instance of the type (Base) which has a signature (Signature) to a callback wrapper.  
  template <typename BaseType, typename Signature >
  std::function<Signature> bat_mem_fun_binder(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {    
    return std::bind(ptr_to_mem, &instance);
  }

  template <typename BaseType, typename Signature >
  std::function<Signature> bat_mem_fun_binder1(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    using namespace std::placeholders;    
    return std::bind(ptr_to_mem, &instance, _1);    
  }

  template <typename BaseType, typename Signature >
  std::function<Signature> bat_mem_fun_binder2(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_mem, &instance, _1, _2);
  }

  template <typename BaseType, typename Signature >
  std::function<Signature> bat_mem_fun_binder3(BaseType & instance, Signature BaseType::* ptr_to_mem)
  {
    using namespace std::placeholders;
    return std::bind(ptr_to_mem, &instance, _1, _2, _3);
  }

  //working with C++ function.operator() 
  template <typename ReturnValue, typename Runnable, typename ... Args>
  ReturnValue run_runnable(Runnable & runnable, Args ... args)
  {
    return runnable(args...);
  }
#endif  

  bool getJSONValue(const std::string& fieldName, const std::string& json, std::string & value);

  bool getJSONList(const std::string& fieldName, const std::string& json, std::vector<std::string> & value);

  bool getJSONWalletInfo(const std::string& json, WALLET_INFO_ST& walletInfo,
    std::string& fee_currency, double& fee_amount, unsigned int& days);

  bool getJSONPublisherTimeStamp(const std::string& json, uint64_t& publisherTimestamp);

  bool getJSONPublisherVerified(const std::string& json, bool& verified);  

  bool getJSONUnsignedTx(const std::string& json, UNSIGNED_TX& unsignedTx);

  bool getJSONTransaction(const std::string& json, TRANSACTION_ST& transaction);

  bool getJSONRates(const std::string& json, std::map<std::string, double>& rates);  

  bool getJSONTwitchProperties(const std::string& json, std::vector<std::map<std::string, std::string>>& parts);

  bool getJSONBatchSurveyors(const std::string& json, std::vector<std::string>& surveyors);

  std::vector<uint8_t> generateSeed();

  std::vector<uint8_t> getHKDF(const std::vector<uint8_t>& seed);

  void getPublicKeyFromSeed(const std::vector<uint8_t>& seed, std::vector<uint8_t>& publicKey, std::vector<uint8_t>& secretKey);

  std::string uint8ToHex(const std::vector<uint8_t>& in);

  std::string stringify(std::string* keys, std::string* values, const unsigned int& size);

  std::string stringifyRequestCredentialsSt(const REQUEST_CREDENTIALS_ST& request_credentials);

  std::string stringifyReconcilePayloadSt(const RECONCILE_PAYLOAD_ST& reconcile_payload);  

  std::string stringifyUnsignedTx(const UNSIGNED_TX& unsignedTx);  

  std::vector<uint8_t> getSHA256(const std::string& in);

  std::string getBase64(const std::vector<uint8_t>& in);

  bool getFromBase64(const std::string& in, std::vector<uint8_t> & out);

  // Sign using ed25519 algorithm
  std::string sign(std::string* keys, std::string* values, const unsigned int& size,
    const std::string& keyId, const std::vector<uint8_t>& secretKey);

  uint64_t currentTime();

  void saveState(const CLIENT_STATE_ST& state);

  void loadState(ReadStateCallback callback);

  void savePublisherState(const PUBLISHER_STATE_ST& state);

  void loadPublisherState(ReadPublisherStateCallback callback);
  
  // We have to implement different function for iOS, probably laptop
  void writeStateFile(const std::string& data);
  
  // We have to implement different function for iOS, probably laptop
  void readStateFile(ReadStateCallback callback);
  
  // We have to implement different function for iOS, probably laptop
  void writePublisherStateFile(const std::string& data);
  
  // We have to implement different function for iOS, probably laptop
  void readPublisherStateFile(ReadPublisherStateCallback callback);

  void getUrlQueryParts(const std::string& query, std::map<std::string, std::string>& parts);

  void getTwitchParts(const std::string& query, std::vector<std::map<std::string, std::string>>& parts);

  std::string getMediaId(const std::map<std::string, std::string>& data, const std::string& type);

  std::string getMediaKey(const std::string& mediaId, const std::string& type);

  uint64_t getMediaDuration(const std::map<std::string, std::string>& data, const std::string& mediaKey, const std::string& type);

  // TODO debug
  //static void readEmscripten();
  //static void readEmscriptenInternal();
  //


  struct CURRENT_RECONCILE {
    CURRENT_RECONCILE();
    ~CURRENT_RECONCILE();

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
    SimpleCallback ledgerCallback_;
  };


  //cross-platform functions
  std::string GenerateGUID();
  void encodeURIComponent(const std::string & instr, std::string & outstr);
  void getDbFile(const std::string & id, std::string & pubDbPath);
  void DecodeURLChars(const std::string& input, std::string& output);
} //namespace braveledger_bat_helper

#endif  // BRAVELEDGER_BAT_HELPER_H_


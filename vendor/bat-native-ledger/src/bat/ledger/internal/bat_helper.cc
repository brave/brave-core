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

#include "base/strings/string_split.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/rapidjson_bat_helper.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "third_party/re2/src/re2/re2.h"
#include "tweetnacl.h"  // NOLINT
#include "url/gurl.h"

namespace braveledger_bat_helper {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

bool isProbiValid(const std::string& probi) {
  // probi shouldn't be longer then 44
  if (probi.length() > 44) {
    return false;
  }

  // checks if probi only contains numbers
  return re2::RE2::FullMatch(probi, "^-?[0-9]*$");
}

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

bool getJSONMessage(const std::string& json,
                     std::string* message) {
  DCHECK(message);
  rapidjson::Document d;
  d.Parse(json.c_str());

  if (message && d.HasMember("message")) {
    *message = d["message"].GetString();
    return true;
  }

  return false;
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

std::string sign(
    const std::vector<std::string>& keys,
    const std::vector<std::string>& values,
    const std::string& key_id,
    const std::vector<uint8_t>& secretKey) {
  DCHECK(keys.size() == values.size());

  std::string headers;
  std::string message;
  for (unsigned int i = 0; i < keys.size(); i++) {
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

  return "keyId=\"" + key_id + "\",algorithm=\"" + SIGNATURE_ALGORITHM +
    "\",headers=\"" + headers + "\",signature=\"" + getBase64(signature) + "\"";
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

std::string toLowerCase(std::string word) {
  std::transform(word.begin(), word.end(), word.begin(), ::tolower);
  return word;
}

}  // namespace braveledger_bat_helper

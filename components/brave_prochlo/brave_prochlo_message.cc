/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_prochlo/brave_prochlo_message.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "base/cxx17_backports.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_prochlo/brave_prochlo_crypto.h"
#include "brave/components/brave_prochlo/prochlo_data.h"
#include "brave/components/brave_prochlo/prochlo_message.pb.h"
#include "crypto/sha2.h"

namespace prochlo {

namespace {

// TODO(iefremov): Make it possible to use testing keys.
// TODO(iefremov): Key versioning?
constexpr char kShufflerKey[] = R"(
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEB+tJ1w8nSnusfxfXV1pq+teKmsb+
kH5op6DjhJABBiLWDhTXyLB38noi7BMwNC3fAcrlVAYPj4ejQ8ohHuSSRA==
-----END PUBLIC KEY-----)";

constexpr char kAnalyzerKey[] = R"(
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEQCeVJbcADloHb8bwftIi1UO0smiz
8ObdAFQ8j3U9cMehGqI3zXgS8APvBW/9XxMkb4XWQe+t9h6qHq82P6zcBg==
-----END PUBLIC KEY-----)";

bool MakeProchlomation(uint64_t metric,
                       const uint8_t* data,
                       const uint8_t* crowd_id,
                       ShufflerItem* shuffler_item) {
  DCHECK(data);
  DCHECK(crowd_id);
  DCHECK(shuffler_item);
  // TODO(iefremov): - create patch for adding `brave_p3a`
  // to src/base/trace_event/builtin_categories.h
  // TRACE_EVENT0("brave_p3a", "MakeProchlomation");

  BraveProchloCrypto crypto;

  const std::vector<char> shuffler_key(
      &kShufflerKey[0], &kShufflerKey[0] + base::size(kShufflerKey));
  if (!crypto.load_shuffler_key_from_bytes(shuffler_key)) {
    return false;
  }

  const std::vector<char> analyzer_key(
      &kAnalyzerKey[0], &kAnalyzerKey[0] + base::size(kAnalyzerKey));
  if (!crypto.load_analyzer_key_from_bytes(analyzer_key)) {
    return false;
  }

  // We have to create a Prochlomation and a PlainShufflerItem to encrypt them
  // both into an AnalyzerItea and a ShufflerItem, respectively. We'll stage
  // those here. We can probably do this more efficiently to avoid copies.
  Prochlomation prochlomation;
  PlainShufflerItem plain_shuffler_item;

  // First the prochlomation
  prochlomation.metric = metric;
  memcpy(prochlomation.data, data, kProchlomationDataLength);

  // Then the AnalyzerItem of the PlainShufflerItem
  if (!crypto.EncryptForAnalyzer(prochlomation,
                                 &plain_shuffler_item.analyzer_item)) {
    NOTREACHED();
    return false;
  }

  // Now prepare the PlainShufflerItem
  memcpy(plain_shuffler_item.crowd_id, crowd_id, kCrowdIdLength);

  // And create the ShufflerItem
  if (!crypto.EncryptForShuffler(plain_shuffler_item, shuffler_item)) {
    NOTREACHED();
    return false;
  }

  return true;
}

void InitProchloMessage(uint64_t metric_hash,
                        const ShufflerItem& item,
                        brave_pyxis::PyxisMessage* pyxis_message) {
  DCHECK(pyxis_message);
  brave_pyxis::PyxisValue* value = pyxis_message->add_pyxis_values();
  value->set_ciphertext(item.ciphertext, kPlainShufflerItemLength);
  value->set_tag(item.tag, kTagLength);
  value->set_nonce(item.nonce, kNonceLength);
  value->set_metric_id(metric_hash);
  value->set_client_public_key(item.client_public_key, kPublicKeyLength);
}

}  // namespace

MessageMetainfo::MessageMetainfo() = default;
MessageMetainfo::~MessageMetainfo() = default;

void GenerateProchloMessage(uint64_t metric_hash,
                            uint64_t metric_value,
                            const MessageMetainfo& meta,
                            brave_pyxis::PyxisMessage* pyxis_message) {
  // TODO(iefremov): - create patch for adding `brave_p3a`
  // to src/base/trace_event/builtin_categories.h
  // TRACE_EVENT0("brave_p3a", "GenerateProchloMessage");
  ShufflerItem item;
  uint8_t data[kProchlomationDataLength] = {0};
  uint8_t crowd_id[kCrowdIdLength] = {0};

  // First byte contains the 4 booleans.
  const char daily = 1;
  const char weekly = 0;
  const char monthly = 2;
  const char first = 0;
  data[0] = daily | weekly | monthly | first;
  uint8_t* ptr = data;
  ptr++;

  // Find out years of install and survey.
  base::Time::Exploded exploded;
  meta.date_of_survey.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  const std::string yos = base::NumberToString(exploded.year).substr(2, 4);
  meta.date_of_install.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  const std::string yoi = base::NumberToString(exploded.year).substr(2, 4);

  const std::string metastring =
      "," + meta.country_code + "," + meta.platform + "," + meta.version + "," +
      meta.channel + "," + yoi + base::NumberToString(meta.woi) + "," + yos +
      base::NumberToString(meta.wos) + "," + meta.refcode + ",";

  const std::string metric_value_str = base::NumberToString(metric_value);

  // TODO(iefremov): replace with 'if'?
  CHECK_LE(metastring.size() + metric_value_str.size(),
           kProchlomationDataLength - 1);

  memcpy(ptr, metastring.data(), metastring.size());
  ptr += metastring.size();
  memcpy(ptr, metric_value_str.data(), metric_value_str.size());

  // TODO(iefremov): Salt?
  crypto::SHA256HashString(
      base::NumberToString(metric_hash) + base::NumberToString(metric_value),
      crowd_id, kCrowdIdLength);
  MakeProchlomation(metric_hash, data, crowd_id, &item);

  InitProchloMessage(metric_hash, item, pyxis_message);
}

void GenerateP3AMessage(uint64_t metric_hash,
                        uint64_t metric_value,
                        const MessageMetainfo& meta,
                        brave_pyxis::RawP3AValue* p3a_message) {
  // TODO(iefremov): - create patch for adding `brave_p3a`
  // to src/base/trace_event/builtin_categories.h
  // TRACE_EVENT0("brave_p3a", "GenerateP3AMessage");
  uint8_t data[kProchlomationDataLength] = {0};

  // First byte contains the 4 booleans.
  const char daily = 1;
  const char weekly = 0;
  const char monthly = 2;
  const char first = 0;
  data[0] = daily | weekly | monthly | first;
  uint8_t* ptr = data;
  ptr++;

  // Find out years of install and survey.
  base::Time::Exploded exploded;
  meta.date_of_survey.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  const std::string yos = base::NumberToString(exploded.year).substr(2, 4);
  meta.date_of_install.LocalExplode(&exploded);
  DCHECK_GE(exploded.year, 999);
  const std::string yoi = base::NumberToString(exploded.year).substr(2, 4);

  const std::string metastring =
      "," + meta.country_code + "," + meta.platform + "," + meta.version + "," +
      meta.channel + "," + yoi + base::NumberToString(meta.woi) + "," + yos +
      base::NumberToString(meta.wos) + "," + meta.refcode + ",";

  const std::string metric_value_str = base::NumberToString(metric_value);

  // TODO(iefremov): replace with 'if'?
  CHECK_LE(metastring.size() + metric_value_str.size(),
           kProchlomationDataLength - 1);

  memcpy(ptr, metastring.data(), metastring.size());
  ptr += metastring.size();
  memcpy(ptr, metric_value_str.data(), metric_value_str.size());

  // Init the message.
  p3a_message->set_metric_id(metric_hash);
  p3a_message->set_p3a_info(data, kProchlomationDataLength);
}

void MaybeStripRefcodeAndCountry(prochlo::MessageMetainfo* meta) {
  const std::string& refcode = meta->refcode;
  const std::string& country = meta->country_code;
  constexpr char kRefcodeNone[] = "none";
  constexpr char kCountryOther[] = "other";
  constexpr char kRefcodeOther[] = "other";

  static base::flat_set<std::string> const kLinuxCountries(
      {"US", "FR", "DE", "GB", "IN", "BR", "PL", "NL", "ES", "CA", "IT", "AU",
       "MX", "CH", "RU", "ZA", "SE", "BE", "JP"});

  static base::flat_set<std::string> const kNotableRefcodes(
      {"BRV001", "GDB255", "APP709", "GBW423", "BRT001", "VNI569", "ICO964",
       "ILY758"});

  static base::flat_set<std::string> const kNotableCountries(
      {"FR", "PH", "GB", "IN", "DE", "BR", "CA", "IT", "ES", "NL", "MX", "AU",
       "RU", "JP", "PL", "ID", "KR", "AR"});

  DCHECK(meta);
  if (meta->platform == "linux-bc") {
    // Because Linux has no refcodes, ignore, and if we have more than
    // 3/0.05 = 60 users in a country for a week of install, we can send
    // country.
    meta->refcode = kRefcodeNone;
    if (kLinuxCountries.count(country) == 0) {
      meta->country_code = kCountryOther;
    }
  } else {
    // Now the minimum platform is MacOS at ~3%, so cut off for a group under
    // here becomes 3/(0.05*0.03) = 2000.
    if (country == "US" || country.empty()) {
      const bool us_and_ref =
          country == "US" && (refcode == kRefcodeNone || refcode == "GDB255" ||
                               refcode == "BRV001");
      const bool unknown_and_ref =
          country.empty() &&
          (kNotableRefcodes.count(refcode) > 0 || refcode == kRefcodeNone);

      if (!(us_and_ref || unknown_and_ref)) {
        meta->refcode = kRefcodeOther;
      }
    } else if (kNotableCountries.count(country) > 0) {
      meta->refcode = kRefcodeOther;
    } else {
      meta->country_code = kCountryOther;
      meta->refcode = kRefcodeOther;
    }
  }
}

}  // namespace prochlo

/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_prochlo/brave_prochlo_message.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
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
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEeQ8KZnYLy5Rw7hZbVjIq4SS8RNgE
/IByF9wOYVRdKTE8ONOZG/ewmgyNbt1GckZEygeKIPxlOWMdasUxCb3D9w==
-----END PUBLIC KEY-----)";

constexpr char kAnalyzerKey[] = R"(
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE0hqO10irC/NDiwBr6ZmKkVbfVYDc
D7YOM2+8Ta6sc325MzwXaQLNzz+NjQcO3lUgA/lKqJ4EdjRXqM576IVmAQ==
-----END PUBLIC KEY-----)";

bool MakeProchlomation(uint64_t metric,
                       const uint8_t* data,
                       const uint8_t* crowd_id,
                       ShufflerItem* shuffler_item) {
  DCHECK(data);
  DCHECK(crowd_id);
  DCHECK(shuffler_item);

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
  ShufflerItem item;
  uint8_t data[kProchlomationDataLength] = {0};
  uint8_t crowd_id[kCrowdIdLength] = {0};

  // first byte contains the 4 booleans
  const char daily = 1;
  const char weekly = 0;
  const char monthly = 2;
  const char first = 0;
  data[0] = daily | weekly | monthly | first;
  uint8_t* ptr = data;
  ptr++;

  // Encode metric ID. Store only two bytes for now.
  // TODO(iefremov): I consider this as a temporary solution, though it should
  // be enough for the test period.
  *ptr++ = metric_hash & 0xFF;
  *ptr++ = (metric_hash >> 8) & 0xFF;

  const std::string metastring = "," + meta.platform + "," + meta.version +
                                 "," + meta.channel + "," + meta.woi + ",";

  memcpy(ptr, metastring.data(), metastring.size());
  ptr += metastring.size();

  const std::string metric_value_str = base::NumberToString(metric_value);
  memcpy(ptr, metric_value_str.data(), metric_value_str.size());

  // TODO(iefremov): Salt?
  crypto::SHA256HashString(
      base::NumberToString(metric_hash) + base::NumberToString(metric_value),
      crowd_id, kCrowdIdLength);
  MakeProchlomation(metric_hash, data, crowd_id, &item);

  InitProchloMessage(metric_hash, item, pyxis_message);
}

}  // namespace prochlo

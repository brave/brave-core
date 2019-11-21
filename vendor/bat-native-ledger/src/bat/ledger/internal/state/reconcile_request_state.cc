/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/reconcile_request_state.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAmountKey[] = "amount";
const char kBodyKey[] = "body";
const char kCurrencyKey[] = "currency";
const char kDenominationKey[] = "denomination";
const char kDestinationKey[] = "destination";
const char kDigestKey[] = "digest";
const char kHeadersKey[] = "headers";
const char kOctetsKey[] = "octets";
const char kRequestTypeKey[] = "requestType";
const char kSignatureKey[] = "signature";
const char kSignedTxKey[] = "signedTx";
const char kSurveyorIdKey[] = "surveyorId";
const char kViewingIdKey[] = "viewingId";

}  // namespace

ReconcileRequestState::ReconcileRequestState() = default;

ReconcileRequestState::~ReconcileRequestState() = default;

bool ReconcileRequestState::ToJson(
    JsonWriter* writer,
    const ReconcileRequestProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  if (!properties.type.empty()) {
    writer->String(kRequestTypeKey);
    writer->String(properties.type.c_str());
  }

  writer->String(kSignedTxKey);
  writer->StartObject();

  writer->String(kHeadersKey);
  writer->StartObject();

  writer->String(kDigestKey);
  writer->String(properties.signed_tx_headers_digest.c_str());

  writer->String(kSignatureKey);
  writer->String(properties.signed_tx_headers_signature.c_str());

  writer->EndObject();

  writer->String(kBodyKey);
  writer->StartObject();

  writer->String(kDenominationKey);
  writer->StartObject();

  writer->String(kAmountKey);
  writer->String(properties.signed_tx_body.amount.c_str());

  writer->String(kCurrencyKey);
  writer->String(properties.signed_tx_body.currency.c_str());

  writer->EndObject();

  writer->String(kDestinationKey);
  writer->String(properties.signed_tx_body.destination.c_str());

  writer->EndObject();

  writer->String(kOctetsKey);
  writer->String(properties.signed_tx_octets.c_str());

  writer->EndObject();

  if (!properties.surveyor_id.empty()) {
    writer->String(kSurveyorIdKey);
    writer->String(properties.surveyor_id.c_str());
  }

  if (!properties.viewing_id.empty()) {
    writer->String(kViewingIdKey);
    writer->String(properties.viewing_id.c_str());
  }

  writer->EndObject();

  return true;
}

std::string ReconcileRequestState::ToJson(
    const ReconcileRequestProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/reconcile_request_state.h"
#include "bat/ledger/internal/properties/unsigned_tx_properties.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ReconcileRequestStateTest.*

namespace ledger {

TEST(ReconcileRequestStateTest, ToJsonSerialization) {
  // Arrange
  ReconcileRequestProperties reconcile_request_properties;
  reconcile_request_properties.type = "RequestType";
  reconcile_request_properties.signed_tx_headers_digest =
      "RequestSignedTxHeadersDigest";
  reconcile_request_properties.signed_tx_headers_signature =
      "RequestSignedTxHeadersSignature";

  UnsignedTxProperties unsigned_tx_properties;
  unsigned_tx_properties.amount = "Amount";
  unsigned_tx_properties.currency = "Currency";
  unsigned_tx_properties.destination = "Destination";
  reconcile_request_properties.signed_tx_body = unsigned_tx_properties;

  reconcile_request_properties.signed_tx_octets = "RequestSignedTxOctets";
  reconcile_request_properties.viewing_id = "RequestViewingId";
  reconcile_request_properties.surveyor_id = "RequestSurveyorId";

  // Act
  const ReconcileRequestState reconcile_request_state;
  const std::string json =
      reconcile_request_state.ToJson(reconcile_request_properties);

  // Assert
  const std::string expected_json = "{\"requestType\":\"RequestType\",\"signedTx\":{\"headers\":{\"digest\":\"RequestSignedTxHeadersDigest\",\"signature\":\"RequestSignedTxHeadersSignature\"},\"body\":{\"denomination\":{\"amount\":\"Amount\",\"currency\":\"Currency\"},\"destination\":\"Destination\"},\"octets\":\"RequestSignedTxOctets\"},\"surveyorId\":\"RequestSurveyorId\",\"viewingId\":\"RequestViewingId\"}";  // NOLINT
  EXPECT_EQ(expected_json, json);
}

}  // namespace ledger

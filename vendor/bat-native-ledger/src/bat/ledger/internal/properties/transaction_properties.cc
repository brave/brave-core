/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/transaction_properties.h"

namespace ledger {

TransactionProperties::TransactionProperties()
    : vote_count(0) {}

TransactionProperties::TransactionProperties(
    const TransactionProperties& properties) {
  viewing_id = properties.viewing_id;
  surveyor_id = properties.surveyor_id;
  contribution_rates = properties.contribution_rates;
  contribution_probi = properties.contribution_probi;
  submission_timestamp = properties.submission_timestamp;
  contribution_rates = properties.contribution_rates;
  anonize_viewing_id = properties.anonize_viewing_id;
  registrar_vk = properties.registrar_vk;
  master_user_token = properties.master_user_token;
  surveyor_ids = properties.surveyor_ids;
  vote_count = properties.vote_count;
  transaction_ballots = properties.transaction_ballots;
}

TransactionProperties::~TransactionProperties() = default;

bool TransactionProperties::operator==(
    const TransactionProperties& rhs) const {
  return viewing_id == rhs.viewing_id &&
      surveyor_id == rhs.surveyor_id &&
      contribution_rates == rhs.contribution_rates &&
      contribution_probi == rhs.contribution_probi &&
      submission_timestamp == rhs.submission_timestamp &&
      anonize_viewing_id == rhs.anonize_viewing_id &&
      registrar_vk == rhs.registrar_vk &&
      master_user_token == rhs.master_user_token &&
      surveyor_ids == rhs.surveyor_ids &&
      vote_count == rhs.vote_count &&
      transaction_ballots == rhs.transaction_ballots;
}

bool TransactionProperties::operator!=(
    const TransactionProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger

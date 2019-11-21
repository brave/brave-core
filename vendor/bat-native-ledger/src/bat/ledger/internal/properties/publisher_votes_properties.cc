/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/publisher_votes_properties.h"

namespace ledger {

PublisherVotesProperties::PublisherVotesProperties() = default;

PublisherVotesProperties::PublisherVotesProperties(
    const PublisherVotesProperties& properties) {
  publisher = properties.publisher;
  batch_votes = properties.batch_votes;
}

PublisherVotesProperties::~PublisherVotesProperties() = default;

bool PublisherVotesProperties::operator==(
    const PublisherVotesProperties& rhs) const {
  return publisher == rhs.publisher &&
      batch_votes == rhs.batch_votes;
}

bool PublisherVotesProperties::operator!=(
    const PublisherVotesProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger

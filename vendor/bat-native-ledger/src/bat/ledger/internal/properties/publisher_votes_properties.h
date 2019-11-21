/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_PUBLISHER_VOTES_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_PUBLISHER_VOTES_PROPERTIES_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/properties/publisher_vote_properties.h"

namespace ledger {

struct PublisherVotesProperties {
  PublisherVotesProperties();
  PublisherVotesProperties(
      const PublisherVotesProperties& properties);
  ~PublisherVotesProperties();

  bool operator==(
      const PublisherVotesProperties& rhs) const;

  bool operator!=(
      const PublisherVotesProperties& rhs) const;

  std::string publisher;
  BatchVotes batch_votes;
};

typedef std::vector<PublisherVotesProperties> PublisherVotes;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_PUBLISHER_VOTES_PROPERTIES_H_

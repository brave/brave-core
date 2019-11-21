/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_PUBLISHER_VOTE_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_PUBLISHER_VOTE_PROPERTIES_H_

#include <string>
#include <vector>

namespace ledger {

struct PublisherVoteProperties {
  PublisherVoteProperties();
  PublisherVoteProperties(
      const PublisherVoteProperties& properties);
  ~PublisherVoteProperties();

  bool operator==(
      const PublisherVoteProperties& rhs) const;

  bool operator!=(
      const PublisherVoteProperties& rhs) const;

  std::string surveyor_id;
  std::string proof;
};

typedef std::vector<PublisherVoteProperties> BatchVotes;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_PUBLISHER_VOTE_PROPERTIES_H_

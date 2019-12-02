/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_PUBLISHER_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_PUBLISHER_PROPERTIES_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace ledger {

struct PublisherProperties {
  PublisherProperties();
  PublisherProperties(
      const PublisherProperties& properties);
  ~PublisherProperties();

  bool operator==(
      const PublisherProperties& rhs) const;

  bool operator!=(
      const PublisherProperties& rhs) const;

  bool operator<(
      const PublisherProperties& rhs) const;

  std::string id;
  uint64_t duration;
  double score;
  uint32_t visits;
  uint32_t percent;
  double weight;
  uint32_t status;
};

typedef std::vector<PublisherProperties> Publishers;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_PUBLISHER_PROPERTIES_H_

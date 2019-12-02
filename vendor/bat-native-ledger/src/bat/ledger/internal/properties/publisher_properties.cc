/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/publisher_properties.h"

namespace ledger {

PublisherProperties::PublisherProperties()
    : duration(0),
      score(0),
      visits(0),
      percent(0),
      weight(0),
      status(0) {}

PublisherProperties::PublisherProperties(
    const PublisherProperties& properties) {
  id = properties.id;
  duration = properties.duration;
  score = properties.score;
  visits = properties.visits;
  percent = properties.percent;
  weight = properties.weight;
  status = properties.status;
}

PublisherProperties::~PublisherProperties() = default;

bool PublisherProperties::operator==(
    const PublisherProperties& rhs) const {
  return id == rhs.id &&
      duration == rhs.duration &&
      score == rhs.score &&
      visits == rhs.visits &&
      percent == rhs.percent &&
      weight == rhs.weight &&
      status == rhs.status;
}

bool PublisherProperties::operator!=(
    const PublisherProperties& rhs) const {
  return !(*this == rhs);
}

bool PublisherProperties::operator<(
    const PublisherProperties& rhs) const {
  return score > rhs.score;
}

}  // namespace ledger

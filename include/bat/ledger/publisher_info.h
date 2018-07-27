/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PUBLISHER_INFO_HANDLER_
#define BAT_LEDGER_PUBLISHER_INFO_HANDLER_

#include <string>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT enum PublisherInfoFilter {
  ALL = 0,
  UNPINNED = 1 << 0,
  PINNED = 1 << 1,
  INCLUDED = 1 << 2,
  DEFAULT = UNPINNED & PINNED & INCLUDED,
};

LEDGER_EXPORT struct PublisherInfo {
  typedef std::string id_type;
  PublisherInfo(const id_type& publisher_id);
  PublisherInfo(const PublisherInfo& info);

  static const PublisherInfo FromJSON(const std::string& json);
  const std::string ToJSON() const;
  bool Matches(PublisherInfoFilter filter) const;
  bool is_valid() const;

  const id_type id;
  uint64_t duration;
  double score;
  uint32_t visits;
  bool pinned;
  uint32_t percent;
  double weight;
  bool excluded;
};

using PublisherInfoList = std::vector<const PublisherInfo>;

}  // namespace ledger

#endif  // BAT_LEDGER_PUBLISHER_INFO_HANDLER_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PUBLISHER_INFO_HANDLER_
#define BAT_LEDGER_PUBLISHER_INFO_HANDLER_

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "bat/ledger/export.h"

namespace ledger {

const char kClearFavicon[] = "clear";
const char kIgnorePublisherBlob[] = "ignore";

LEDGER_EXPORT enum ACTIVITY_MONTH {
  ANY = -1,
  JANUARY = 1,
  FEBRUARY = 2,
  MARCH = 3,
  APRIL = 4,
  MAY = 5,
  JUNE = 6,
  JULY = 7,
  AUGUST = 8,
  SEPTEMBER = 9,
  OCTOBER = 10,
  NOVEMBER = 11,
  DECEMBER = 12
};

LEDGER_EXPORT enum PUBLISHER_EXCLUDE {
  ALL = -1,
  DEFAULT = 0,  // this tell us that user did not manually changed exclude state
  EXCLUDED = 1,  // user manually changed it to exclude
  INCLUDED = 2  // user manually changed it to include and is overriding server
};

}  // namespace ledger

#endif  // BAT_LEDGER_PUBLISHER_INFO_HANDLER_

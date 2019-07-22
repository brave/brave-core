/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_MEDIA_EVENT_INFO_H_
#define BAT_LEDGER_MEDIA_EVENT_INFO_H_

#include <string>

#include "bat/ledger/export.h"
#include "bat/ledger/publisher_info.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using MediaEventInfo = mojom::MediaEventInfo;
using MediaEventInfoPtr = mojom::MediaEventInfoPtr;

}  // namespace ledger

#endif  // BAT_LEDGER_MEDIA_EVENT_INFO_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/media/youtube.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_media {

MediaYouTube::MediaYouTube(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger) {
}

MediaYouTube::~MediaYouTube() {
}


}  // namespace braveledger_media

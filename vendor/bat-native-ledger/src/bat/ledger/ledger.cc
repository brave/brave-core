/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/media/media.h"
#include "bat/ledger/internal/legacy/static_values.h"
#include "bat/ledger/internal/constants.h"

namespace ledger {

type::Environment _environment = type::Environment::PRODUCTION;

bool is_debug = false;
bool is_testing = false;
int reconcile_interval = 0;  // minutes
int retry_interval = 0;      // seconds

// static
Ledger* Ledger::CreateInstance(LedgerClient* client) {
  return new LedgerImpl(client);
}

bool Ledger::IsMediaLink(const std::string& url,
                         const std::string& first_party_url,
                         const std::string& referrer) {
  const std::string type = braveledger_media::Media::GetLinkType(
      url,
      first_party_url,
      referrer);

  return type == TWITCH_MEDIA_TYPE || type == VIMEO_MEDIA_TYPE;
}

}  // namespace ledger

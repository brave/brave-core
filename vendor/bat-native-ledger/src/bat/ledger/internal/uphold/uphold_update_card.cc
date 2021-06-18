/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_update_card.h"

namespace ledger {
namespace uphold {

UpdateCard::UpdateCard() : label(""), position(-1), starred(false) {}

UpdateCard::~UpdateCard() = default;

}  // namespace uphold
}  // namespace ledger

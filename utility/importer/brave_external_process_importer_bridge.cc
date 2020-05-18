/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/utility/importer/brave_external_process_importer_bridge.h"

BraveExternalProcessImporterBridge::
    ~BraveExternalProcessImporterBridge() = default;

void BraveExternalProcessImporterBridge::SetCreditCard(
    const base::string16& name_on_card,
    const base::string16& expiration_month,
    const base::string16& expiration_year,
    const base::string16& decrypted_card_number,
    const std::string& origin) {
  observer_->OnCreditCardImportReady(name_on_card, expiration_month,
                                     expiration_year, decrypted_card_number,
                                     origin);
}


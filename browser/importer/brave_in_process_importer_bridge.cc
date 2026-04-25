/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_in_process_importer_bridge.h"

#include "chrome/browser/importer/profile_writer.h"

BraveInProcessImporterBridge::~BraveInProcessImporterBridge() = default;

void BraveInProcessImporterBridge::SetCreditCard(
    const std::u16string& name_on_card,
    const std::u16string& expiration_month,
    const std::u16string& expiration_year,
    const std::u16string& decrypted_card_number,
    const std::string& origin) {
  writer_->AddCreditCard(name_on_card,
                         expiration_month,
                         expiration_year,
                         decrypted_card_number,
                         origin);
}

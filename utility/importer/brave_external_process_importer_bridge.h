/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

#include <string>

#include "chrome/utility/importer/external_process_importer_bridge.h"

class BraveExternalProcessImporterBridge
    : public ExternalProcessImporterBridge {
 public:
  using ExternalProcessImporterBridge::ExternalProcessImporterBridge;

  BraveExternalProcessImporterBridge(
      const BraveExternalProcessImporterBridge&) = delete;
  BraveExternalProcessImporterBridge& operator=(
      const BraveExternalProcessImporterBridge&) = delete;

  void SetCreditCard(const base::string16& name_on_card,
                     const base::string16& expiration_month,
                     const base::string16& expiration_year,
                     const base::string16& decrypted_card_number,
                     const std::string& origin) override;

 private:
  ~BraveExternalProcessImporterBridge() override;
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

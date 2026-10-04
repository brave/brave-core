/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_IN_PROCESS_IMPORTER_BRIDGE_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_IN_PROCESS_IMPORTER_BRIDGE_H_

#include <string>

#include "brave/common/importer/brave_importer_bridge.h"
#include "chrome/browser/importer/in_process_importer_bridge.h"

class BraveInProcessImporterBridge : public InProcessImporterBridge,
                                     public BraveImporterBridge {
 public:
  using InProcessImporterBridge::InProcessImporterBridge;

  BraveInProcessImporterBridge(const BraveInProcessImporterBridge&) = delete;
  BraveInProcessImporterBridge operator=(
      const BraveInProcessImporterBridge&) = delete;

  // BraveImporterBridge overrides:
  void SetCreditCard(const std::u16string& name_on_card,
                     const std::u16string& expiration_month,
                     const std::u16string& expiration_year,
                     const std::u16string& decrypted_card_number,
                     const std::string& origin) override;

 private:
  ~BraveInProcessImporterBridge() override;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_IN_PROCESS_IMPORTER_BRIDGE_H_

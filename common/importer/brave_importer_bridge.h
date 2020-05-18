/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_IMPORTER_BRIDGE_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_IMPORTER_BRIDGE_H_

#include <string>

#include "base/strings/string16.h"
#include "chrome/common/importer/importer_bridge.h"

class BraveImporterBridge : public ImporterBridge {
 public:
  BraveImporterBridge() {}

  BraveImporterBridge(const BraveImporterBridge&) = delete;
  BraveImporterBridge& operator=(const BraveImporterBridge&) = delete;

  // Used string primitives instead of introducing new struct to minimize mojom
  // patching.
  virtual void SetCreditCard(const base::string16& name_on_card,
                             const base::string16& expiration_month,
                             const base::string16& expiration_year,
                             const base::string16& decrypted_card_number,
                             const std::string& origin) = 0;

 protected:
  friend class ImporterBridge;

  ~BraveImporterBridge() override {}
};

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_IMPORTER_BRIDGE_H_

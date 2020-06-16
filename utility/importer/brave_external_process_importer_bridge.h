/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

#include <string>

#include "brave/common/importer/brave_importer_bridge.h"
#include "brave/common/importer/profile_import.mojom.h"
#include "chrome/utility/importer/external_process_importer_bridge.h"

class BraveExternalProcessImporterBridge : public ExternalProcessImporterBridge,
                                           public BraveImporterBridge {
 public:
  BraveExternalProcessImporterBridge(
      const base::flat_map<uint32_t, std::string>& localized_strings,
      mojo::SharedRemote<chrome::mojom::ProfileImportObserver> observer,
      mojo::SharedRemote<brave::mojom::ProfileImportObserver> brave_observer);

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

  mojo::SharedRemote<brave::mojom::ProfileImportObserver> brave_observer_;
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

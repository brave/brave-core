/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

#include <string>

#include "chrome/browser/importer/external_process_importer_client.h"

class BraveExternalProcessImporterClient
    : public ExternalProcessImporterClient {
 public:
  using ExternalProcessImporterClient::ExternalProcessImporterClient;

  BraveExternalProcessImporterClient(
      const BraveExternalProcessImporterClient&) = delete;
  BraveExternalProcessImporterClient& operator=(
      const BraveExternalProcessImporterClient&) = delete;

  void OnCreditCardImportReady(
      const base::string16& name_on_card,
      const base::string16& expiration_month,
      const base::string16& expiration_year,
      const base::string16& decrypted_card_number,
      const std::string& origin) override;

 protected:
  ~BraveExternalProcessImporterClient() override;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

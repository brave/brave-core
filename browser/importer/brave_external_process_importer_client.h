/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/common/importer/profile_import.mojom.h"
#include "chrome/browser/importer/external_process_importer_client.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class BraveExternalProcessImporterClient
    : public ExternalProcessImporterClient,
      public brave::mojom::ProfileImportObserver {
 public:
  BraveExternalProcessImporterClient(
      base::WeakPtr<ExternalProcessImporterHost> importer_host,
      const importer::SourceProfile& source_profile,
      uint16_t items,
      InProcessImporterBridge* bridge);

  BraveExternalProcessImporterClient(
      const BraveExternalProcessImporterClient&) = delete;
  BraveExternalProcessImporterClient& operator=(
      const BraveExternalProcessImporterClient&) = delete;

  // ExternalProcessImportClient overrides:
  void Start() override;
  void Cancel() override;
  void CloseMojoHandles() override;
  void OnImportItemFinished(importer::ImportItem import_item) override;

  // brave::mojom::ProfileImportObserver overrides:
  void OnCreditCardImportReady(const std::u16string& name_on_card,
                               const std::u16string& expiration_month,
                               const std::u16string& expiration_year,
                               const std::u16string& decrypted_card_number,
                               const std::string& origin) override;

 protected:
  ~BraveExternalProcessImporterClient() override;

 private:
  // Used to start and stop the actual brave importer running in a different
  // process.
  mojo::Remote<brave::mojom::ProfileImport> brave_profile_import_;

  // Used to receive progress updates from the brave importer.
  mojo::Receiver<brave::mojom::ProfileImportObserver> brave_receiver_{this};
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

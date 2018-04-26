/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

#include <vector>

#include "brave/browser/importer/brave_in_process_importer_bridge.h"
#include "chrome/browser/importer/external_process_importer_client.h"
#include "net/cookies/canonical_cookie.h"

class BraveExternalProcessImporterClient : public ExternalProcessImporterClient {
 public:
  BraveExternalProcessImporterClient(
      base::WeakPtr<ExternalProcessImporterHost> importer_host,
      const importer::SourceProfile& source_profile,
      uint16_t items,
      BraveInProcessImporterBridge* bridge);

  // Called by the ExternalProcessImporterHost on import cancel.
  void Cancel();

  void OnCookiesImportStart(
      uint32_t total_cookies_count) override;
  void OnCookiesImportGroup(
      const std::vector<net::CanonicalCookie>& cookies_group) override;

 private:
  ~BraveExternalProcessImporterClient() override;

  // Total number of cookies to import.
  size_t total_cookies_count_;

  scoped_refptr<BraveInProcessImporterBridge> bridge_;

  std::vector<net::CanonicalCookie> cookies_;

  // True if import process has been cancelled.
  bool cancelled_;

  DISALLOW_COPY_AND_ASSIGN(BraveExternalProcessImporterClient);
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

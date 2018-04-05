/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

#include <vector>

#include "chrome/utility/importer/external_process_importer_bridge.h"
#include "net/cookies/canonical_cookie.h"

class BraveExternalProcessImporterBridge :
                                      public ExternalProcessImporterBridge {
 public:
  // |observer| must outlive this object.
  BraveExternalProcessImporterBridge(
      const base::DictionaryValue& localized_strings,
      scoped_refptr<chrome::mojom::ThreadSafeProfileImportObserverPtr>
          observer);

  void SetCookies(const std::vector<net::CanonicalCookie>& cookies) override;
 private:
  ~BraveExternalProcessImporterBridge() override;

  DISALLOW_COPY_AND_ASSIGN(BraveExternalProcessImporterBridge);
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

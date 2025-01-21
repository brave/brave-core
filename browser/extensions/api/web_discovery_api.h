/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_WEB_DISCOVERY_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_WEB_DISCOVERY_API_H_

#include <string>

#include "brave/components/brave_search/browser/backup_results_service.h"
#include "extensions/browser/extension_function.h"

namespace extensions::api {

class WebDiscoveryRetrieveBackupResultsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("webDiscovery.retrieveBackupResults", UNKNOWN)

 protected:
  ~WebDiscoveryRetrieveBackupResultsFunction() override;
  ResponseAction Run() override;

 private:
  void HandleBackupResults(
      std::optional<brave_search::BackupResultsService::BackupResults> results);
};

}  // namespace extensions::api

#endif  // BRAVE_BROWSER_EXTENSIONS_API_WEB_DISCOVERY_API_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_EXTENSION_WHITELIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_EXTENSION_WHITELIST_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

class ExtensionWhitelistParser;
class BraveExtensionProviderTest;
class BravePDFDownloadTest;

namespace brave_component_updater {

// The brave shields service in charge of extension whitelist
class ExtensionWhitelistService : public LocalDataFilesObserver {
 public:
  using GetDATFileDataResult =
      brave_component_updater::LoadDATFileDataResult<ExtensionWhitelistParser>;

  explicit ExtensionWhitelistService(
      LocalDataFilesService* local_data_files_service,
      const std::vector<std::string>& whitelist);
  ExtensionWhitelistService(const ExtensionWhitelistService&) = delete;
  ExtensionWhitelistService& operator=(const ExtensionWhitelistService&) =
      delete;
  ~ExtensionWhitelistService() override;

  bool IsWhitelisted(const std::string& extension_id) const;
  bool IsBlacklisted(const std::string& extension_id) const;
  bool IsVetted(const std::string& extension_id) const;

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::BraveExtensionProviderTest;
  friend class ::BravePDFDownloadTest;

  void OnGetDATFileData(GetDATFileDataResult result);

  SEQUENCE_CHECKER(sequence_checker_);
  std::unique_ptr<ExtensionWhitelistParser> extension_whitelist_client_;
  brave_component_updater::DATFileDataBuffer buffer_;
  std::vector<std::string> whitelist_;
  base::WeakPtrFactory<ExtensionWhitelistService> weak_factory_;
};

// Creates the ExtensionWhitelistService
std::unique_ptr<ExtensionWhitelistService> ExtensionWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service,
    const std::vector<std::string>& whitelist);

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_EXTENSION_WHITELIST_SERVICE_H_

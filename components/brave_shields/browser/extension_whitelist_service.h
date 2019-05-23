/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_EXTENSION_WHITELIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_EXTENSION_WHITELIST_SERVICE_H_

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

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

// TODO(bridiver) - move out of brave shields
namespace brave_shields {

// The brave shields service in charge of extension whitelist
class ExtensionWhitelistService : public LocalDataFilesObserver {
 public:
  using GetDATFileDataResult =
      std::pair<std::unique_ptr<ExtensionWhitelistParser>,
                                brave_component_updater::DATFileDataBuffer>;

  explicit ExtensionWhitelistService(
      LocalDataFilesService* local_data_files_service);
  ~ExtensionWhitelistService() override;

  bool IsWhitelisted(const std::string& extension_id) const;
  bool IsBlacklisted(const std::string& extension_id) const;

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::BraveExtensionProviderTest;
  friend class ::BravePDFDownloadTest;

  void OnGetDATFileData(GetDATFileDataResult result);

  std::unique_ptr<ExtensionWhitelistParser> extension_whitelist_client_;
  brave_component_updater::DATFileDataBuffer buffer_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ExtensionWhitelistService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionWhitelistService);
};

// Creates the ExtensionWhitelistService
std::unique_ptr<ExtensionWhitelistService> ExtensionWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_EXTENSION_WHITELIST_SERVICE_H_

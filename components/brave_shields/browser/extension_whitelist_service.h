/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_EXTENSION_WHITELIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_EXTENSION_WHITELIST_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

class ExtensionWhitelistParser;
class BraveExtensionProviderTest;
class BravePDFDownloadTest;

namespace brave_shields {

// The brave shields service in charge of extension whitelist
class ExtensionWhitelistService : public BaseLocalDataFilesObserver {
 public:
  ExtensionWhitelistService();
  ~ExtensionWhitelistService() override;

  bool IsWhitelisted(const std::string& extension_id) const;
  bool IsBlacklisted(const std::string& extension_id) const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // implementation of BaseLocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::BraveExtensionProviderTest;
  friend class ::BravePDFDownloadTest;

  void OnDATFileDataReady();

  brave_component_updater::DATFileDataBuffer buffer_;

  std::unique_ptr<ExtensionWhitelistParser> extension_whitelist_client_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ExtensionWhitelistService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(ExtensionWhitelistService);
};

// Creates the ExtensionWhitelistService
std::unique_ptr<ExtensionWhitelistService> ExtensionWhitelistServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_EXTENSION_WHITELIST_SERVICE_H_

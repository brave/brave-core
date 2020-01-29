/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AUTOPLAY_WHITELIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AUTOPLAY_WHITELIST_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "url/gurl.h"

class AutoplayWhitelistParser;
class BraveContentSettingsAgentImplAutoplayTest;

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace brave_shields {

// The brave shields service in charge of autoplay whitelist
class AutoplayWhitelistService : public LocalDataFilesObserver {
 public:
  using GetDATFileDataResult =
      brave_component_updater::LoadDATFileDataResult<AutoplayWhitelistParser>;

  explicit AutoplayWhitelistService(
      LocalDataFilesService* local_data_files_service);
  ~AutoplayWhitelistService() override;

  bool ShouldAllowAutoplay(const GURL& url);

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::BraveContentSettingsAgentImplAutoplayTest;

  void OnGetDATFileData(GetDATFileDataResult result);

  std::unique_ptr<AutoplayWhitelistParser> autoplay_whitelist_client_;
  brave_component_updater::DATFileDataBuffer buffer_;
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AutoplayWhitelistService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AutoplayWhitelistService);
};

// Creates the AutoplayWhitelistService
std::unique_ptr<AutoplayWhitelistService> AutoplayWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AUTOPLAY_WHITELIST_SERVICE_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "url/gurl.h"

namespace brave_user_agent {

class BraveUserAgentService
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit BraveUserAgentService(
      brave_component_updater::LocalDataFilesService* local_data_files_service);

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  bool CanShowBrave(const GURL& url);
  ~BraveUserAgentService() override;
  void SetIsReadyForTesting() { is_ready_ = true; }
  void OnDATFileDataReady(const std::string& contents);

 private:
  void LoadBraveUserAgentedDomains(const base::FilePath& install_dir);
  std::set<std::string> exceptional_domains_;
  bool is_ready_ = false;
  base::WeakPtrFactory<BraveUserAgentService> weak_factory_{this};
};

// Creates the BraveUserAgentService
std::unique_ptr<BraveUserAgentService> BraveUserAgentServiceFactory(
    brave_component_updater::LocalDataFilesService* local_data_files_service);

}  // namespace brave_user_agent

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_SERVICE_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
// TODO(brave): <mutex> is an unapproved C++11 header
#include <mutex>  // NOLINT
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

class CTPParser;
class TrackingProtectionServiceTest;

namespace brave_shields {

// The brave shields service in charge of tracking protection and init.
class TrackingProtectionService : public BaseLocalDataFilesObserver {
 public:
  TrackingProtectionService();
  ~TrackingProtectionService() override;

  bool ShouldStartRequest(const GURL& spec,
                          content::ResourceType resource_type,
                          const std::string& tab_host,
                          bool* matching_exception_filter,
                          bool* cancel_request_explicitly);
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // implementation of BaseLocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  void OnDATFileDataReady();
  std::vector<std::string> GetThirdPartyHosts(const std::string& base_host);

  brave_shields::DATFileDataBuffer buffer_;

  std::unique_ptr<CTPParser> tracking_protection_client_;
  std::vector<std::string> third_party_base_hosts_;
  std::map<std::string, std::vector<std::string>> third_party_hosts_cache_;
  std::mutex third_party_hosts_mutex_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<TrackingProtectionService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(TrackingProtectionService);
};

// Creates the TrackingProtectionService
std::unique_ptr<TrackingProtectionService> TrackingProtectionServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_

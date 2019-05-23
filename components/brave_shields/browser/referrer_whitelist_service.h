/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_REFERRER_WHITELIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_REFERRER_WHITELIST_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

#define REFERRER_DAT_FILE "ReferrerWhitelist.json"
#define REFERRER_DAT_FILE_VERSION "1"

class ReferrerWhitelistServiceTest;

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace brave_shields {

// The brave shields service in charge of referrer whitelist
class ReferrerWhitelistService : public LocalDataFilesObserver {
 public:
  explicit ReferrerWhitelistService(
      LocalDataFilesService* local_data_files_service);
  ~ReferrerWhitelistService() override;

  bool IsWhitelisted(const GURL& firstPartyOrigin,
                     const GURL& subresourceUrl) const;

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::ReferrerWhitelistServiceTest;

  struct ReferrerWhitelist {
    URLPattern first_party_pattern;
    URLPatternList subresource_pattern_list;
    ReferrerWhitelist();
    ReferrerWhitelist(const ReferrerWhitelist& other);
    ~ReferrerWhitelist();
  };

  bool IsWhitelisted(const std::vector<ReferrerWhitelist>& whitelist,
                     const GURL& first_party_origin,
                     const GURL& subresource_url) const;
  void OnDATFileDataReady(std::string contents);
  void OnDATFileDataReadyOnIOThread(std::vector<ReferrerWhitelist> whitelist);

  typedef std::vector<URLPattern> URLPatternList;

  std::vector<ReferrerWhitelist> referrer_whitelist_;
  std::vector<ReferrerWhitelist> referrer_whitelist_io_thread_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ReferrerWhitelistService> weak_factory_;
  base::WeakPtrFactory<ReferrerWhitelistService> weak_factory_io_thread_;
  DISALLOW_COPY_AND_ASSIGN(ReferrerWhitelistService);
};

// Creates the ReferrerWhitelistService
std::unique_ptr<ReferrerWhitelistService> ReferrerWhitelistServiceFactory(
    LocalDataFilesService* local_data_files_service);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_REFERRER_WHITELIST_SERVICE_H_

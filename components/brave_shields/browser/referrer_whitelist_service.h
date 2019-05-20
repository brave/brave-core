/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_REFERRER_WHITELIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_REFERRER_WHITELIST_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"
#include "content/public/common/resource_type.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

#define REFERRER_DAT_FILE "ReferrerWhitelist.json"
#define REFERRER_DAT_FILE_VERSION "1"

class ReferrerWhitelistServiceTest;

namespace brave_shields {

// The brave shields service in charge of referrer whitelist
class ReferrerWhitelistService : public BaseLocalDataFilesObserver {
 public:
  ReferrerWhitelistService();
  ~ReferrerWhitelistService() override;

  bool IsWhitelisted(const GURL& firstPartyOrigin,
                     const GURL& subresourceUrl) const;
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // implementation of BaseLocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  friend class ::ReferrerWhitelistServiceTest;

  void OnDATFileDataReady();

  typedef std::vector<URLPattern> URLPatternList;

  struct ReferrerWhitelist {
    URLPattern first_party_pattern;
    URLPatternList subresource_pattern_list;
    ReferrerWhitelist();
    ReferrerWhitelist(const ReferrerWhitelist& other);
    ~ReferrerWhitelist();
  };

  std::string file_contents_;
  std::vector<ReferrerWhitelist> referrer_whitelist_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ReferrerWhitelistService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(ReferrerWhitelistService);
};

// Creates the ReferrerWhitelistService
std::unique_ptr<ReferrerWhitelistService> ReferrerWhitelistServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_REFERRER_WHITELIST_SERVICE_H_

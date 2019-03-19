/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_BASE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_BASE_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_shields/browser/base_brave_shields_service.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "content/public/common/resource_type.h"

class AdBlockClient;
class AdBlockServiceTest;

namespace brave_shields {

// The base class of the brave shields service in charge of ad-block
// checking and init.
class AdBlockBaseService : public BaseBraveShieldsService {
 public:
  AdBlockBaseService();
  ~AdBlockBaseService() override;

  bool ShouldStartRequest(const GURL &url, content::ResourceType resource_type,
    const std::string& tab_host, bool* did_match_exception,
    bool* cancel_request_explicitly) override;
  void EnableTag(const std::string& tag, bool enabled);

 protected:
  friend class ::AdBlockServiceTest;
  bool Init() override;
  void Cleanup() override;

  void EnableTagOnFileTaskRunner(std::string tag, bool enabled);
  void GetDATFileData(const base::FilePath& dat_file_path);
  AdBlockClient* GetAdBlockClientForTest();

  std::unique_ptr<AdBlockClient> ad_block_client_;
  DATFileDataBuffer buffer_;

 private:
  void OnDATFileDataReady();
  void OnPreferenceChanges(const std::string& pref_name);

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<AdBlockBaseService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AdBlockBaseService);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_BASE_SERVICE_H_

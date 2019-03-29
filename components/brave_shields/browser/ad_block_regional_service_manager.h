/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/synchronization/lock.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

namespace base {
class ListValue;
}  // namespace base

class AdBlockServiceTest;

namespace brave_shields {

class AdBlockRegionalService;

// The AdBlock regional service manager, in charge of initializing and
// managing regional AdBlock clients.
class AdBlockRegionalServiceManager {
 public:
  AdBlockRegionalServiceManager();
  ~AdBlockRegionalServiceManager();

  static bool IsSupportedLocale(const std::string& locale);
  static std::unique_ptr<base::ListValue> GetRegionalLists();

  bool IsInitialized() const;
  bool Start();
  void Stop();
  bool ShouldStartRequest(const GURL& url,
                          content::ResourceType resource_type,
                          const std::string& tab_host,
                          bool* matching_exception_filter,
                          bool* cancel_request_explicitly);
  void EnableTag(const std::string& tag, bool enabled);
  void EnableFilterList(const std::string& uuid, bool enabled);
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

 private:
  friend class ::AdBlockServiceTest;
  bool Init();
  void StartRegionalServices();
  void UpdateFilterListPrefs(const std::string& uuid, bool enabled);

  bool initialized_;
  base::Lock regional_services_lock_;
  std::map<std::string, std::unique_ptr<AdBlockRegionalService>>
      regional_services_;

  DISALLOW_COPY_AND_ASSIGN(AdBlockRegionalServiceManager);
};

// Creates the AdBlockRegionalServiceManager
std::unique_ptr<AdBlockRegionalServiceManager>
AdBlockRegionalServiceManagerFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

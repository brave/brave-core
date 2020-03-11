/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

namespace base {
class ListValue;
}  // namespace base

class AdBlockServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

class AdBlockRegionalService;

// The AdBlock regional service manager, in charge of initializing and
// managing regional AdBlock clients.
class AdBlockRegionalServiceManager {
 public:
  explicit AdBlockRegionalServiceManager(BraveComponent::Delegate* delegate);
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
                          bool* cancel_request_explicitly,
                          std::string* mock_data_url);
  void EnableTag(const std::string& tag, bool enabled);
  void AddResources(const std::string& resources);
  void EnableFilterList(const std::string& uuid, bool enabled);

  base::Optional<base::Value> HostnameCosmeticResources(
          const std::string& hostname);
  base::Optional<base::Value> HiddenClassIdSelectors(
          const std::vector<std::string>& classes,
          const std::vector<std::string>& ids,
          const std::vector<std::string>& exceptions);

 private:
  friend class ::AdBlockServiceTest;
  bool Init();
  void StartRegionalServices();
  void UpdateFilterListPrefs(const std::string& uuid, bool enabled);

  brave_component_updater::BraveComponent::Delegate* delegate_;  // NOT OWNED
  bool initialized_;
  base::Lock regional_services_lock_;
  std::map<std::string, std::unique_ptr<AdBlockRegionalService>>
      regional_services_;

  DISALLOW_COPY_AND_ASSIGN(AdBlockRegionalServiceManager);
};

// Creates the AdBlockRegionalServiceManager
std::unique_ptr<AdBlockRegionalServiceManager>
AdBlockRegionalServiceManagerFactory(BraveComponent::Delegate* delegate);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_REGIONAL_SERVICE_MANAGER_H_

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <mutex>

#include "brave/components/brave_shields/browser/base_brave_shields_service.h"
#include "content/public/common/resource_type.h"

class CTPParser;

namespace brave_shields {

// The brave shields service in charge of ad-block checking and init.
class TrackingProtectionService : public BaseBraveShieldsService {
 public:
  TrackingProtectionService();
  ~TrackingProtectionService() override;

  bool Check(const GURL &spec,
    content::ResourceType resource_type,
    const std::string &initiator_host) override;

 protected:
  bool Init() override;
  void Cleanup() override;

 private:
  std::vector<std::string> GetThirdPartyHosts(const std::string& base_host);

  std::vector<unsigned char> buffer_;
  std::unique_ptr<CTPParser> tracking_protection_client_;
  // TODO: Temporary hack which matches both browser-laptop and Android code
  std::vector<std::string> white_list_;
  std::vector<std::string> third_party_base_hosts_;
  std::map<std::string, std::vector<std::string>> third_party_hosts_cache_;
  std::mutex third_party_hosts_mutex_;
};

// Creates the TrackingProtectionService
std::unique_ptr<BaseBraveShieldsService> TrackingProtectionServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_

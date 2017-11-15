/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "brave/components/brave_shields/browser/base_brave_shields_service.h"
#include "content/public/common/resource_type.h"

class AdBlockClient;

namespace brave_shields {

// The brave shields service in charge of ad-block checking and init.
class AdBlockService : public BaseBraveShieldsService {
 public:
   AdBlockService();
   ~AdBlockService() override;

   bool Check(const std::string &spec,
    content::ResourceType resource_type,
    const std::string &initiator_host) override;

 protected:
   bool Init() override;
   void Cleanup() override;

 private:
  std::vector<unsigned char> adblock_buffer_;
  std::unique_ptr<AdBlockClient> ad_block_client_;
};

// Creates the AdBlockService
std::unique_ptr<BaseBraveShieldsService> AdBlockServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_

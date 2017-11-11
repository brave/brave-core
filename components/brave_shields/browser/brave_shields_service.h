/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "content/public/common/resource_type.h"

class AdBlockClient;

namespace brave_shields {

// The brave shields service in charge of checking brave shields like ad-block.
class BraveShieldsService {
 public:
   BraveShieldsService();
   ~BraveShieldsService();
   bool Start();
   void Stop();
   bool Check(const std::string &spec,
    content::ResourceType resource_type,
    const std::string &initiator_host);

 private:
  bool InitAdBlock();
  bool GetData(const std::string& fileName,
      std::vector<unsigned char>& buffer);
  void set_adblock_initialized();

  bool initialized_;
  std::vector<unsigned char> adblock_buffer_;
  std::mutex adblock_init_mutex_;
  std::mutex initialized_mutex_;
  std::unique_ptr<AdBlockClient> ad_block_client_;
};

// Creates the BraveShieldsService.
std::unique_ptr<BraveShieldsService> BraveShieldsServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SERVICE_H_

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_shields/browser/ad_block_base_service.h"
#include "content/public/common/resource_type.h"

class AdBlockClient;
class AdBlockServiceTest;

namespace brave_shields {

const std::string kAdBlockComponentName("Brave Ad Block Updater");
const std::string kAdBlockComponentId("cffkpbalmllkdoenhmdmpbkajipdjfam");

const std::string kAdBlockComponentBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs0qzJmHSgIiw7IGFCxij"
    "1NnB5hJ5ZQ1LKW9htL4EBOaMJvmqaDs/wfq0nw/goBHWsqqkMBynRTu2Hxxirvdb"
    "cugn1Goys5QKPgAvKwDHJp9jlnADWm5xQvPQ4GE1mK1/I3ka9cEOCzPW6GI+wGLi"
    "VPx9VZrxHHsSBIJRaEB5Tyi5bj0CZ+kcfMnRTsXIBw3C6xJgCVKISQUkd8mawVvG"
    "vqOhBOogCdb9qza5eJ1Cgx8RWKucFfaWWxKLOelCiBMT1Hm1znAoVBHG/blhJJOD"
    "5HcH/heRrB4MvrE1J76WF3fvZ03aHVcnlLtQeiNNOZ7VbBDXdie8Nomf/QswbBGa"
    "VwIDAQAB";

// The brave shields service in charge of ad-block checking and init.
class AdBlockService : public AdBlockBaseService {
 public:
   AdBlockService();
   ~AdBlockService() override;

  bool ShouldStartRequest(const GURL &url,
    content::ResourceType resource_type,
    const std::string& tab_host) override;

 protected:
  bool Init() override;
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir) override;

 private:
  friend class ::AdBlockServiceTest;
  static std::string g_ad_block_component_id_;
  static std::string g_ad_block_component_base64_public_key_;
  static std::string g_ad_block_dat_file_version_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);
  static void SetDATFileVersionForTest(const std::string& dat_file_version);

  DISALLOW_COPY_AND_ASSIGN(AdBlockService);
};

// Creates the AdBlockService
std::unique_ptr<AdBlockService> AdBlockServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_

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

#include "base/files/file_path.h"
#include "brave/components/brave_shields/browser/base_brave_shields_service.h"
#include "content/public/common/resource_type.h"

class AdBlockClient;
class AdBlockServiceTest;

namespace brave_shields {

const std::string kAdBlockUpdaterName("Brave Ad Block Updater");
const std::string kAdBlockUpdaterId("cffkpbalmllkdoenhmdmpbkajipdjfam");

const std::string kAdBlockUpdaterBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs0qzJmHSgIiw7IGFCxij"
    "1NnB5hJ5ZQ1LKW9htL4EBOaMJvmqaDs/wfq0nw/goBHWsqqkMBynRTu2Hxxirvdb"
    "cugn1Goys5QKPgAvKwDHJp9jlnADWm5xQvPQ4GE1mK1/I3ka9cEOCzPW6GI+wGLi"
    "VPx9VZrxHHsSBIJRaEB5Tyi5bj0CZ+kcfMnRTsXIBw3C6xJgCVKISQUkd8mawVvG"
    "vqOhBOogCdb9qza5eJ1Cgx8RWKucFfaWWxKLOelCiBMT1Hm1znAoVBHG/blhJJOD"
    "5HcH/heRrB4MvrE1J76WF3fvZ03aHVcnlLtQeiNNOZ7VbBDXdie8Nomf/QswbBGa"
    "VwIDAQAB";

// The brave shields service in charge of ad-block checking and init.
class AdBlockService : public BaseBraveShieldsService {
 public:
   AdBlockService();
   ~AdBlockService() override;

  bool ShouldStartRequest(const GURL &url,
    content::ResourceType resource_type,
    const std::string& tab_host) override;

 protected:
  bool Init() override;
  void Cleanup() override;
  void OnComponentRegistered(const std::string& extension_id) override;
  void OnComponentReady(const std::string& extension_id,
                        const base::FilePath& install_dir) override;

 private:
  friend class ::AdBlockServiceTest;
  static std::string g_ad_block_updater_id_;
  static std::string g_ad_block_updater_base64_public_key_;
  static void SetIdAndBase64PublicKeyForTest(
      const std::string& id,
      const std::string& base64_public_key);

  std::vector<unsigned char> buffer_;
  std::unique_ptr<AdBlockClient> ad_block_client_;
};

// Creates the AdBlockService
std::unique_ptr<AdBlockService> AdBlockServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_SERVICE_H_

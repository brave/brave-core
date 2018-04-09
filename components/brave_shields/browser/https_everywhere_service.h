/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYHWERE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYHWERE_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "base/files/file_path.h"
#include "brave/components/brave_shields/browser/base_brave_shields_service.h"
#include "brave/components/brave_shields/browser/https_everywhere_recently_used_cache.h"
#include "content/public/common/resource_type.h"

namespace leveldb {
class DB;
}

class HTTPSEverywhereServiceTest;

namespace brave_shields {

const std::string kHTTPSEverywhereUpdaterBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvn9zSMjTmhkQyrZu5UdN"
    "350nPqLoSeCYngcC7yDFwaUHjoBQXCZqGeDC69ciCQ2mlRhcV2nxXqlUDkiC6+7m"
    "651nI+gi4oVqHagc7EFUyGA0yuIk7qIMvCBdH7wbET27de0rzbRzRht9EKzEjIhC"
    "BtoPnmyrO/8qPrH4XR4cPfnFPuJssBBxC1B35H7rh0Br9qePhPDDe9OjyqYxPuio"
    "+YcC9obL4g5krVrfrlKLfFNpIewUcJyBpSlCgfxEyEhgDkK9cILTMUi5vC7GxS3P"
    "OtZqgfRg8Da4i+NwmjQqrz0JFtPMMSyUnmeMj+mSOL4xZVWr8fU2/GOCXs9gczDp"
    "JwIDAQAB";

const std::string kHTTPSEverywhereUpdaterId("oofiananboodjbbmdelgdommihjbkfag");
const std::string kHTTPSEverywhereUpdaterName("HTTPS Everywhere Updater");

struct HTTPSE_REDIRECTS_COUNT_ST {
public:
    HTTPSE_REDIRECTS_COUNT_ST(uint64_t request_identifier,
        unsigned int redirects):
      request_identifier_(request_identifier),
      redirects_(redirects) {
    }

    uint64_t request_identifier_;
    unsigned int redirects_;
};

class HTTPSEverywhereService : public BaseBraveShieldsService {
 public:
   HTTPSEverywhereService();
   ~HTTPSEverywhereService() override;
  bool GetHTTPSURL(const GURL* url, const uint64_t& request_id,
      std::string& new_url);
  bool GetHTTPSURLFromCacheOnly(const GURL* url,
      const uint64_t& request_id, std::string& cached_url);

 protected:
  bool Init() override;
  void Cleanup() override;

  void AddHTTPSEUrlToRedirectList(const uint64_t& request_id);
  bool ShouldHTTPSERedirect(const uint64_t& request_id);
  std::string ApplyHTTPSRule(const std::string& originalUrl,
      const std::string& rule);
  std::string CorrecttoRuleToRE2Engine(const std::string& to);

 private:
  friend class ::HTTPSEverywhereServiceTest;
  void OnComponentRegistered(const std::string& extension_id);
  void OnComponentReady(
    const std::string& extension_id,
    const base::FilePath& install_dir);

  std::mutex httpse_get_urls_redirects_count_mutex_;
  std::vector<HTTPSE_REDIRECTS_COUNT_ST> httpse_urls_redirects_count_;
  HTTPSERecentlyUsedCache<std::string> recently_used_cache_;
  leveldb::DB* level_db_;
};

// Creates the HTTPSEverywhereService
std::unique_ptr<HTTPSEverywhereService> HTTPSEverywhereServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_HTTPS_EVERYHWERE_SERVICE_H_

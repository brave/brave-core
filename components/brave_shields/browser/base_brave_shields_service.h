/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "content/public/common/resource_type.h"
#include "url/gurl.h"

namespace brave_shields {

class DATFileWebRequest;

// The brave shields service in charge of checking brave shields like ad-block,
// tracking protection, etc.
class BaseBraveShieldsService {
 public:
  BaseBraveShieldsService(const std::string &file_name, const GURL &url);
  virtual ~BaseBraveShieldsService();
  bool Start();
  void Stop();
  virtual bool ShouldStartRequest(const GURL &url,
      content::ResourceType resource_type,
      const std::string &initiator_host);
 protected:
  virtual bool Init() = 0;
  virtual void Cleanup() = 0;

 private:
  void DownloadDATFile();
  void InitShields();

  void DATFileResponse(bool success);

  const std::string file_name_;
  const GURL url_;
  bool initialized_;
  std::unique_ptr<DATFileWebRequest> web_request_;
  std::mutex init_mutex_;
  std::mutex initialized_mutex_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_

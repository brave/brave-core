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

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

// Just used to give access to OnDemandUpdater since it's private.
// Chromium has ComponentsUI which is a friend class, so we just
// do this hack here to gain access.
class ComponentsUI {
 public:
  void OnDemandUpdate(
      component_updater::ComponentUpdateService* cus,
      const std::string& component_id);
};

namespace brave_shields {

// The brave shields service in charge of checking brave shields like ad-block,
// tracking protection, etc.
class BaseBraveShieldsService : public ComponentsUI {
 public:
  BaseBraveShieldsService(const std::string& component_name,
                          const std::string& component_id,
                          const std::string& component_base64_public_key);
  virtual ~BaseBraveShieldsService();
  bool Start();
  void Stop();
  bool IsInitialized() const;
  virtual bool ShouldStartRequest(const GURL& url,
      content::ResourceType resource_type,
      const std::string& tab_host);
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return task_runner_;
  }

 protected:
  virtual bool Init() = 0;
  virtual void Cleanup() = 0;
  virtual void OnComponentRegistered(const std::string& component_id);
  virtual void OnComponentReady(const std::string& component_id,
                                const base::FilePath& install_dir);

 private:
  void InitShields();

  bool initialized_;
  std::mutex initialized_mutex_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::string component_name_;
  std::string component_id_;
  std::string component_base64_public_key_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_BRAVE_SHIELDS_SERVICE_H_

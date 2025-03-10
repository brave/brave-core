/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "url/gurl.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_user_agent {

class BraveUserAgentService {
 public:
  explicit BraveUserAgentService(
      component_updater::ComponentUpdateService* cus);

  bool CanShowBrave(const GURL& url);
  ~BraveUserAgentService();
  void SetIsReadyForTesting() { is_ready_ = true; }

 private:
  void OnComponentReady(const base::FilePath&);
  void OnExceptionalDomainsLoaded(const std::string& contents);
  base::FilePath component_path_;
  std::set<std::string> exceptional_domains_;
  bool is_ready_ = false;
  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;
  base::WeakPtrFactory<BraveUserAgentService> weak_factory_{this};
};

}  // namespace brave_user_agent

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_SERVICE_H_

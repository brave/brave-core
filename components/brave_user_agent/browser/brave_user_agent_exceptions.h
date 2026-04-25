/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_EXCEPTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_EXCEPTIONS_H_

#include <set>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_user_agent {

class BraveUserAgentExceptions {
 public:
  BraveUserAgentExceptions(const BraveUserAgentExceptions&) = delete;
  BraveUserAgentExceptions& operator=(const BraveUserAgentExceptions&) = delete;
  ~BraveUserAgentExceptions();
  static BraveUserAgentExceptions* GetInstance();  // singleton

  void OnComponentReady(const base::FilePath&);
  bool CanShowBrave(const GURL& url);
  void SetIsReadyForTesting() { is_ready_ = true; }
  void AddToExceptedDomainsForTesting(std::string_view domain);

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveUserAgentExceptionsUnitTest,
                           TestCanShowBraveDomainsLoaded);
  BraveUserAgentExceptions();
  void OnExceptedDomainsLoaded(const std::string& contents);

  base::FilePath component_path_;
  std::set<std::string> excepted_domains_;
  bool is_ready_ = false;
  raw_ptr<component_updater::ComponentUpdateService> component_update_service_;
  base::WeakPtrFactory<BraveUserAgentExceptions> weak_factory_{this};

  friend struct base::DefaultSingletonTraits<BraveUserAgentExceptions>;
  friend class BraveUserAgentExceptionsUnitTest;
};

}  // namespace brave_user_agent

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_EXCEPTIONS_H_

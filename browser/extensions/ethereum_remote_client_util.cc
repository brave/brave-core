/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/ethereum_remote_client_util.h"

#include <memory>

#include "base/environment.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/constants/brave_services_key.h"

namespace extensions {

bool HasInfuraProjectID() {
  std::string project_id = GetInfuraProjectID();

  if (!project_id.empty()) {
    return true;
  }

  std::unique_ptr<base::Environment> env(base::Environment::Create());
  bool has_project_id = env->HasVar("BRAVE_INFURA_PROJECT_ID");

  return has_project_id;
}

std::string GetInfuraProjectID() {
  std::string project_id(BUILDFLAG(BRAVE_INFURA_PROJECT_ID));
  std::unique_ptr<base::Environment> env(base::Environment::Create());

  if (env->HasVar("BRAVE_INFURA_PROJECT_ID")) {
    env->GetVar("BRAVE_INFURA_PROJECT_ID", &project_id);
  }

  return project_id;
}

std::string GetBraveKey() {
  std::string brave_key(BUILDFLAG(BRAVE_SERVICES_KEY));
  std::unique_ptr<base::Environment> env(base::Environment::Create());

  if (env->HasVar("BRAVE_SERVICES_KEY")) {
    env->GetVar("BRAVE_SERVICES_KEY", &brave_key);
  }

  return brave_key;
}

}  // namespace extensions

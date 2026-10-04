// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/webui_url_constants.h"

// Include password_manager constants before override so we don't override the
// definition.
#include "components/password_manager/content/common/web_ui_constants.h"

// Make sure IsUrlEligibleForWebApp also checks for allowed Brave WebUI hosts
#define IsUrlEligibleForWebApp IsUrlEligibleForWebApp_ChromiumImpl

#include <components/webapps/browser/web_app_url_config.cc>

#undef IsUrlEligibleForWebApp

namespace webapps {

bool IsUrlEligibleForWebApp(const GURL& app_url) {
  return IsUrlEligibleForWebApp_ChromiumImpl(app_url) ||
         (app_url.SchemeIs(content::kChromeUIScheme) &&
          kInstallablePWAWebUIHosts.contains(app_url.host()));
}

}  // namespace webapps

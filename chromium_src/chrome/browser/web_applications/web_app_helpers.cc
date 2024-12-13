// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/containers/contains.h"
#include "brave/components/constants/webui_url_constants.h"

// Make sure IsValidWebAppUrl also checks for allowed Brave WebUI hosts
#define IsValidWebAppUrl IsValidWebAppUrl_ChromiumImpl

#include "src/chrome/browser/web_applications/web_app_helpers.cc"
#undef IsValidWebAppUrl

namespace web_app {

bool IsValidWebAppUrl(const GURL& app_url) {
  return IsValidWebAppUrl_ChromiumImpl(app_url) ||
         (app_url.SchemeIs(content::kChromeUIScheme) &&
          base::Contains(kInstallablePWAWebUIHosts, app_url.host_piece()));
}

}  // namespace web_app
